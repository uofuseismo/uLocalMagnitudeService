#include <chrono>
#include <exception>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/support/status.h>
#include <grpcpp/support/server_callback.h>
#include <grpcpp/support/time.h> //NOLINT
#include <grpcpp/impl/channel_argument_option.h>
#include <grpc/impl/compression_types.h>
//NOLINTNEXTLINE(misc-include-cleaner)
#include <spdlog/sinks/stdout_color_sinks.h>
#include "uLocalMagnitudeService/grpc/magnitudeService.hpp"
#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"
#include "uLocalMagnitudeService/grpc/magnitudeServiceOptions.hpp"
#include "uLocalMagnitudeService/grpc/serverOptions.hpp"
#include "uLocalMagnitudeService/magnitude/networkMagnitude.hpp"
#include "uLocalMagnitudeService/magnitude/networkMagnitudeCalculator.hpp"
#include "uLocalMagnitudeService/magnitude/networkMagnitudeCalculatorOptions.hpp"
#include "uLocalMagnitudeService/metrics/singleton.hpp"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/service.grpc.pb.h"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/distance_corrections_request.pb.h"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/distance_corrections_response.pb.h"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/station_corrections_request.pb.h"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/station_corrections_response.pb.h"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/station_magnitudes_from_amplitudes_request.pb.h"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/station_magnitudes_from_amplitudes_response.pb.h"
#include "validateClient.hpp"

using namespace ULocalMagnitudeService::GRPC;

namespace ULMSAPIV1 = ULocalMagnitudeServiceAPI::V1::Magnitude;

class MagnitudeService::MagnitudeServiceImpl :
    public ULMSAPIV1::MagnitudeService::CallbackService
{
public:
    /// @brief Constructor
    MagnitudeServiceImpl
    (
        const MagnitudeServiceOptions &options,
        std::shared_ptr<spdlog::logger> logger
    ) :
        mOptions(options),
        mLogger(std::move(logger))
    {
        if (!mOptions.hasGRPCOptions())
        {
            throw std::invalid_argument("gRPC options not set for server");
        }
        if (!mOptions.hasNetworkMagnitudeCalculatorOptions())
        {
            throw std::invalid_argument(
                "Network magnitude calculator options not set");
        }
        if (mLogger == nullptr)
        {
            // NOLINTBEGIN(misc-include-cleaner)
            constexpr const char *loggerName{"ServiceConsole"};
            mLogger = spdlog::get(loggerName);
            if (mLogger == nullptr)
            {
                mLogger = spdlog::stdout_color_mt(loggerName);
            }
            // NOLINTEND(misc-include-cleaner)
        }
        mCalculator
            = std::make_unique<Magnitude::NetworkMagnitudeCalculator> (
                 mOptions.getNetworkMagnitudeCalculatorOptions(),
                 mLogger);
        if (mCalculator == nullptr)
        {
            throw std::runtime_error(
                "Failed to create network magnitude calculator");
        }
        if (!mCalculator->isInitialized())
        {
            throw std::runtime_error(
                "Failed to initialize network magnitude calculator");
        }
    }

    ///----------------------------------------------------------------------///
    /// @brief Station corrections                                           ///
    ///----------------------------------------------------------------------///
    grpc::ServerUnaryReactor
        *GetStationCorrections(
            grpc::CallbackServerContext *context,
            const ULMSAPIV1::StationCorrectionsRequest *request,
            ULMSAPIV1::StationCorrectionsResponse *response) override
    {
        class Reactor : public grpc::ServerUnaryReactor
        {
        public:
            Reactor(grpc::CallbackServerContext *context,
                    const ULMSAPIV1::StationCorrectionsRequest &request,
                    ULMSAPIV1::StationCorrectionsResponse *response,
                    const GRPC::ServerOptions &grpcOptions,
                    const bool isSecured, 
                    const Magnitude::NetworkMagnitudeCalculator &calculator,
                    std::shared_ptr<spdlog::logger> logger) :
                mLogger(std::move(logger))
            {
                auto &metrics = Metrics::Singleton::getInstance();
                // Validate the client?
                if (isSecured)
                {
                    auto accessToken = grpcOptions.getAccessToken();
                    if (accessToken)
                    {
                        if (!::validateClient(context, *accessToken))
                        {
                            SPDLOG_LOGGER_WARN(mLogger,
                                              "Unauthorized client {} rejected",
                                               context->peer());
                            metrics.incrementUnauthenticatedCounter(mRouteName);
                            Finish({grpc::StatusCode::UNAUTHENTICATED,
                                    "Invalid access token"});
                            return;
                        }
                    }
                }
                // Copy the request identifier
                std::string requestIdentifier{context->peer()};
                if (request.has_identifier())
                {
                    requestIdentifier = requestIdentifier
                                      + " ("
                                      + request.identifier()
                                      + ")";
                    *response->mutable_identifier() = request.identifier();
                }
                SPDLOG_LOGGER_DEBUG(mLogger,
                                    "Getting station corrections for {}",
                                    requestIdentifier);
                // Do it
                try
                {
                    namespace UCorrections
                        = ULocalMagnitudeService::Corrections;
                    for (const auto &grpcStationIdentifier :
                         request.station_identifiers())
                    {
                        try
                        {
                             const UCorrections::StationIdentifier
                                 identifier{grpcStationIdentifier};
                             auto correction
                                 = calculator.getStationCorrection(identifier);
                             ULMSAPIV1::StationCorrectionsResponse
                                      ::StationCorrection stationCorrection;
                             *stationCorrection.mutable_station_identifier()
                                 = grpcStationIdentifier;
                             if (correction)
                             {
                                 stationCorrection.set_correction(*correction);
                                 stationCorrection.set_exists(true);
                             }
                             else
                             {
                                 stationCorrection.set_correction(0);
                                 stationCorrection.set_exists(false);
                             }
                        }
                        catch (const std::invalid_argument &e)
                        {
                            Finish({grpc::StatusCode::INVALID_ARGUMENT,
                                    "Malformed station identifier"});
                            return;
                        }
                        catch (const std::exception &e)
                        {
                            SPDLOG_LOGGER_WARN(
                               mLogger,
                               "Failed to get station corrections because {}",
                               std::string{e.what()});
                            Finish({grpc::StatusCode::INTERNAL,
                               "Server error - contact developer"});
                            return;
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    metrics.incrementServerErrorCounter(mRouteName);
                    SPDLOG_LOGGER_WARN(
                        mLogger,
                        "Failed to get station corrections because {}",
                        std::string{e.what()});
                    Finish({grpc::StatusCode::INTERNAL,
                            "Server error - try using a different endpoint"});
                    return;
                }
                mSuccess = true;
                Finish(grpc::Status::OK);
                SPDLOG_LOGGER_DEBUG(
                    mLogger,
                    "Successfully found station corrections for {}",
                    requestIdentifier);
            }
        private:
            void OnDone() override
            {
                if (mLogger)
                {
                    SPDLOG_LOGGER_DEBUG(mLogger,
                                        "GetStationCorrections RPC completed");
                }
                auto &metrics = Metrics::Singleton::getInstance();
                if (mSuccess)
                {
                    metrics.incrementSuccessCounter(mRouteName);
                    const auto endTime = std::chrono::steady_clock::now();
                    auto elapsedTime
                        = std::chrono::duration<double>
                          (mRPCStartTime - endTime);
                    metrics.recordRouteDuration(elapsedTime, mRouteName);
                }
                delete this;
            }
            void OnCancel() override
            {
                if (mLogger)
                {
                   SPDLOG_LOGGER_DEBUG(mLogger,
                                       "GetStationCorrections RPC canceled");
                }
            }
//private:
            std::shared_ptr<spdlog::logger> mLogger{nullptr};
            const std::chrono::time_point<std::chrono::steady_clock> mRPCStartTime
            {
                std::chrono::steady_clock::now()
            };
            const std::string mRouteName{"GetStationCorrections"};
            bool mSuccess{false};
        };
        return new Reactor(context,
                           *request,
                           response,
                           mGRPCOptions,
                           mSecured,
                           *mCalculator,
                           mLogger);
    }
//private:
    MagnitudeServiceOptions mOptions;
    std::shared_ptr<spdlog::logger> mLogger{nullptr};
    std::unique_ptr<Magnitude::NetworkMagnitudeCalculator> mCalculator{nullptr};
    GRPC::ServerOptions mGRPCOptions;
    bool mSecured{false};
};


/// Destructor
MagnitudeService::~MagnitudeService() = default;

