#include <chrono>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
//NOLINTNEXTLINE(misc-include-cleaner)
#include <spdlog/sinks/stdout_color_sinks.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/status.h>
#include <grpcpp/support/server_callback.h>
#include <grpcpp/support/time.h> //NOLINT
//#include <grpc/impl/compression_types.h>
#include "magnitudeService.hpp"
#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"
#include "uLocalMagnitudeService/corrections/distance.hpp"
#include "uLocalMagnitudeService/corrections/hypocenter.hpp"
#include "uLocalMagnitudeService/grpc/magnitudeServiceOptions.hpp"
#include "uLocalMagnitudeService/magnitude/networkMagnitude.hpp"
#include "uLocalMagnitudeService/magnitude/networkMagnitudeCalculator.hpp"
#include "uLocalMagnitudeService/magnitude/networkMagnitudeCalculatorOptions.hpp"
#include "uLocalMagnitudeService/magnitude/stationMagnitude.hpp"
#include "uLocalMagnitudeService/magnitude/observation.hpp"
#include "uLocalMagnitudeService/metrics/singleton.hpp"
#include "uLocalMagnitudeService/version.hpp"
//NOLINTNEXTLINE(misc-include-cleaner)
#include "uLocalMagnitudeServiceAPI/v1/magnitude/service.grpc.pb.h"
//#include "uLocalMagnitudeServiceAPI/v1/magnitude/distance_corrections_request.pb.h"
//#include "uLocalMagnitudeServiceAPI/v1/magnitude/distance_corrections_response.pb.h"
//NOLINTNEXTLINE(misc-include-cleaner)
#include "uLocalMagnitudeServiceAPI/v1/magnitude/hypocenter.pb.h"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/network_magnitude_from_amplitudes_request.pb.h"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/network_magnitude_from_amplitudes_response.pb.h"
//NOLINTNEXTLINE(misc-include-cleaner)
#include "uLocalMagnitudeServiceAPI/v1/magnitude/station_amplitude_measurement.pb.h"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/station_corrections_request.pb.h"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/station_corrections_response.pb.h"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/station_magnitude.pb.h"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/station_magnitudes_from_amplitudes_request.pb.h"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/station_magnitudes_from_amplitudes_response.pb.h"
#include "validateClient.hpp"

using namespace ULocalMagnitudeService::GRPC;

namespace ULMSAPIV1 = ULocalMagnitudeServiceAPI::V1::Magnitude;

class MagnitudeService::MagnitudeServiceImpl
{
public:
    /// @brief Constructor
    MagnitudeServiceImpl
    (
        const Magnitude::NetworkMagnitudeCalculatorOptions &calculatorOptions,
        std::optional<std::string> &accessToken,
        std::shared_ptr<spdlog::logger> logger
    ) :
        mAccessToken(accessToken),
        mLogger(std::move(logger))
    {
        if (mLogger == nullptr)
        {
            // NOLINTBEGIN(misc-include-cleaner)
            constexpr const char *loggerName{"MagnitudeServiceConsole"};
            mLogger = spdlog::get(loggerName);
            if (mLogger == nullptr)
            {
                mLogger = spdlog::stdout_color_mt(loggerName);
            }
            // NOLINTEND(misc-include-cleaner)
        }
        mCalculator
            = std::make_unique<Magnitude::NetworkMagnitudeCalculator> (
                 calculatorOptions,
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

    /// @brief Constructor
//private:
    std::unique_ptr<Magnitude::NetworkMagnitudeCalculator> mCalculator{nullptr};
    std::optional<std::string> mAccessToken{std::nullopt};
    std::shared_ptr<spdlog::logger> mLogger{nullptr};
};

/// Constructor
MagnitudeService::MagnitudeService(
    const Magnitude::NetworkMagnitudeCalculatorOptions &calculatorOptions,
    std::optional<std::string> &accessToken,
    std::shared_ptr<spdlog::logger> logger) :
    pImpl(std::make_unique<MagnitudeServiceImpl>
          (
              calculatorOptions,
              accessToken,
              std::move(logger)
          )
         )
{
}


/// Destructor
MagnitudeService::~MagnitudeService() = default;

///--------------------------------------------------------------------------///
///                           Network magnitude                              ///
///--------------------------------------------------------------------------///
grpc::ServerUnaryReactor
*MagnitudeService::ComputeNetworkMagnitudeFromAmplitudes(
    grpc::CallbackServerContext *context,
    const ULMSAPIV1::NetworkMagnitudeFromAmplitudesRequest *request,
    ULMSAPIV1::NetworkMagnitudeFromAmplitudesResponse *response)    
{
    class Reactor : public grpc::ServerUnaryReactor
    {
    public:
        Reactor(grpc::CallbackServerContext *context,
                const ULMSAPIV1::NetworkMagnitudeFromAmplitudesRequest &request,
                ULMSAPIV1::NetworkMagnitudeFromAmplitudesResponse *response,
                const std::optional<std::string> &accessToken,
                const Magnitude::NetworkMagnitudeCalculator &calculator,
                std::shared_ptr<spdlog::logger> logger) :
            mLogger(std::move(logger))
        {
            auto &metrics = Metrics::Singleton::getInstance();
            // Validate the client?
            if (accessToken != std::nullopt)
            {
                if (!::validateClient(context, *accessToken))
                {
                    SPDLOG_LOGGER_WARN(mLogger,
                                       "Unauthorized client - {} rejected",
                                       context->peer());
                    metrics.incrementUnauthenticatedCounter(mRouteName);
                    Finish({grpc::StatusCode::UNAUTHENTICATED,
                            "Invalid access token"});
                    return;
                }
            }
            // Copy the request identifier
            std::string requestIdentifier{context->peer()};
            *response->mutable_version()
                = ULocalMagnitudeService::Version::getVersionWithTag();
            if (request.has_identifier())
            {
                requestIdentifier = requestIdentifier
                                  + " ("
                                  + request.identifier()
                                  + ")";
                *response->mutable_identifier() = request.identifier();
            }
            SPDLOG_LOGGER_INFO(mLogger,
                               "Computing network magnitude for {}",
                               requestIdentifier);
            //----------------------------------------------------------------//
            // Step 1: Pack the observations                                  //
            //----------------------------------------------------------------//
            std::vector<Magnitude::Observation> observations;
            try
            {
                if (!request.has_hypocenter())
                {
                    metrics.incrementClientErrorCounter(mRouteName);
                    Finish({grpc::StatusCode::INVALID_ARGUMENT,
                            "Malformed request - hypocenter not set"});
                    return;
                }
                const Corrections::Hypocenter hypocenter{request.hypocenter()};
                for (const auto &grpcMeasurement :
                     request.station_amplitude_measurements())
                {
                    try
                    {
                        Magnitude::Observation observation
                        {   
                            grpcMeasurement,
                            hypocenter
                        };
                        observations.push_back(std::move(observation));
                    }
                    catch (const std::invalid_argument &e)
                    {
                        SPDLOG_LOGGER_WARN(
                            mLogger,
                            "Failed to create observation because {}",
                            e.what());
                        metrics.incrementClientErrorCounter(mRouteName);
                        Finish({grpc::StatusCode::INVALID_ARGUMENT,
                                "Check hypocenter and "
                              + std::to_string(observations.size())
                              + " measurement"});
                        return;
                    } 
                    catch (const std::exception &e)
                    {
                        SPDLOG_LOGGER_ERROR(
                            mLogger,
                            "Failed to create observation because {}", 
                            e.what());
                        metrics.incrementServerErrorCounter(mRouteName);
                        Finish({grpc::StatusCode::INTERNAL,
                                "Server error - contact maintainer"});
                        return;
                    }    
                }
            }
            catch (const std::invalid_argument &e)
            {
                SPDLOG_LOGGER_WARN(
                    mLogger,
                    "Failed to create hypocenter because {}",
                    std::string{e.what()});
                metrics.incrementClientErrorCounter(mRouteName);
                Finish({grpc::StatusCode::INVALID_ARGUMENT,
                        "Check hypocenter"});
                return;
            }
            catch (const std::exception &e) 
            {
                metrics.incrementServerErrorCounter(mRouteName);
                SPDLOG_LOGGER_ERROR(
                    mLogger,
                    "Failed to unpack observations because {}",
                    std::string{e.what()});
                Finish({grpc::StatusCode::INTERNAL,
                        "Server error - try using a different endpoint"});
                return;
            }
            if (observations.empty())
            {
                metrics.incrementClientErrorCounter(mRouteName);
                Finish({grpc::StatusCode::INVALID_ARGUMENT, "No observations"});
                return;
            }
            ///--------------------------------------------------------------///
            /// Step 2: Compute Magnitude                                    ///
            ///--------------------------------------------------------------///

            mSuccess = true;
            Finish(grpc::Status::OK);
            SPDLOG_LOGGER_INFO(
                mLogger,
                "Successfully computed network magnitude for {}",
                requestIdentifier);
        }
    private:
        void OnDone() override
        {
            if (mLogger)
            {
                SPDLOG_LOGGER_DEBUG(mLogger,
                                    "{} RPC completed", mRouteName);
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
                                   "{} RPC canceled", mRouteName);
            }
        }
//private:
        std::shared_ptr<spdlog::logger> mLogger{nullptr};
        const std::chrono::time_point<std::chrono::steady_clock> mRPCStartTime
        {
            std::chrono::steady_clock::now()
        };
        const std::string mRouteName
        {
            "ComputeNetworkMagnitudeFromAmplitudes"
        };
        bool mSuccess{false};
    };
    return new Reactor(context,
                       *request,
                       response,
                       pImpl->mAccessToken,
                       *pImpl->mCalculator,
                       pImpl->mLogger);
}

///--------------------------------------------------------------------------///
///                           Station magnitudes                             ///
///--------------------------------------------------------------------------///
grpc::ServerUnaryReactor
*MagnitudeService::ComputeStationMagnitudesFromAmplitudes(
    grpc::CallbackServerContext *context,
    const ULMSAPIV1::StationMagnitudesFromAmplitudesRequest *request,
    ULMSAPIV1::StationMagnitudesFromAmplitudesResponse *response)
{
    class Reactor : public grpc::ServerUnaryReactor
    {
    public:
        Reactor(grpc::CallbackServerContext *context,
                const ULMSAPIV1::StationMagnitudesFromAmplitudesRequest &request,
                ULMSAPIV1::StationMagnitudesFromAmplitudesResponse *response,
                const std::optional<std::string> &accessToken,
                const Magnitude::NetworkMagnitudeCalculator &calculator,
                std::shared_ptr<spdlog::logger> logger) :
            mLogger(std::move(logger))
        {
            auto &metrics = Metrics::Singleton::getInstance();
            // Validate the client?
            if (accessToken != std::nullopt)
            {
                if (!::validateClient(context, *accessToken))
                {
                    SPDLOG_LOGGER_WARN(mLogger,
                                       "Unauthorized client - {} rejected",
                                       context->peer());
                    metrics.incrementUnauthenticatedCounter(mRouteName);
                    Finish({grpc::StatusCode::UNAUTHENTICATED,
                            "Invalid access token"});
                    return;
                }
            }
            // Copy the request identifier
            std::string requestIdentifier{context->peer()};
            *response->mutable_version()
                = ULocalMagnitudeService::Version::getVersionWithTag();
            if (request.has_identifier())
            {
                requestIdentifier = requestIdentifier
                                  + " ("
                                  + request.identifier()
                                  + ")";
                *response->mutable_identifier() = request.identifier();
            }
            SPDLOG_LOGGER_DEBUG(mLogger,
                                "Computing station magnitudes for {}",
                                requestIdentifier);
            // Do it
            try
            {
                if (!request.has_hypocenter())
                {
                    metrics.incrementClientErrorCounter(mRouteName);
                    Finish({grpc::StatusCode::INVALID_ARGUMENT,
                            "Malformed request - hypocenter not set"});
                    return;
                }
                const Corrections::Hypocenter hypocenter{request.hypocenter()};
                for (const auto &grpcMeasurement :
                     request.station_amplitude_measurements())
                {
                    try
                    {
                        const Magnitude::Observation observation
                        {
                            grpcMeasurement,
                            hypocenter
                        };
                        auto stationMagnitude
                            = calculator.computeStationMagnitude(
                                observation);
                        if (stationMagnitude.has_value())
                        {
                            auto stationMagnitudeMessage
                                = stationMagnitude->toMessage
                                  <ULMSAPIV1::StationMagnitude> ();
                            response->mutable_station_magnitudes()->Add(
                                std::move(stationMagnitudeMessage));
                        }
                    }
                    catch (const std::invalid_argument &e)
                    {
                        SPDLOG_LOGGER_WARN(
                             mLogger,
                             "Failed to compute station mag because {}",
                             e.what());
                        auto index = response->station_magnitudes().size();
                        metrics.incrementClientErrorCounter(mRouteName);
                        Finish({grpc::StatusCode::INVALID_ARGUMENT,
                                + "Measurement " 
                                + std::to_string(index + 1)
                                + " is malformed"});
                        return;
                    }
                    catch (const std::exception &e)
                    {
                        metrics.incrementServerErrorCounter(mRouteName);
                        SPDLOG_LOGGER_ERROR(
                               mLogger,
                               "Failed to get station magnitude because {}",
                               std::string{e.what()});
                        Finish({grpc::StatusCode::INTERNAL,
                                "Server error - contact developer"});
                        return;
                    }
                }
            }
            catch (const std::invalid_argument &e)
            {
                SPDLOG_LOGGER_WARN( 
                    mLogger,
                    "Failed to create hypocenter because {}",
                    std::string{e.what()});
                metrics.incrementClientErrorCounter(mRouteName);
                Finish({grpc::StatusCode::INVALID_ARGUMENT,
                        "Check hypocenter"});
                return;
            }
            catch (const std::exception &e) 
            {
                metrics.incrementServerErrorCounter(mRouteName);
                SPDLOG_LOGGER_ERROR(
                    mLogger,
                    "Failed to get station magnitudes because {}",
                    std::string{e.what()});
                Finish({grpc::StatusCode::INTERNAL,
                        "Server error - try using a different endpoint"});
                return;
            }
            mSuccess = true;
            Finish(grpc::Status::OK);
            SPDLOG_LOGGER_DEBUG(
                mLogger,
                "Successfully computed station magnitudes for {}",
                requestIdentifier);
        }
    private:
        void OnDone() override
        {
            if (mLogger)
            {
                SPDLOG_LOGGER_DEBUG(mLogger,
                                    "{} RPC completed", mRouteName);
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
                                   "{} RPC canceled", mRouteName);
            }
        }
//private:
        std::shared_ptr<spdlog::logger> mLogger{nullptr};
        const std::chrono::time_point<std::chrono::steady_clock> mRPCStartTime
        {
            std::chrono::steady_clock::now()
        };
        const std::string mRouteName
        {
            "ComputeStationMagnitudesFromAmplitudes"
        };
        bool mSuccess{false};
    };
    return new Reactor(context,
                       *request,
                       response,
                       pImpl->mAccessToken,
                       *pImpl->mCalculator,
                       pImpl->mLogger);
}

///--------------------------------------------------------------------------///
///                           Station corrections                            ///
///--------------------------------------------------------------------------///
grpc::ServerUnaryReactor
*MagnitudeService::GetStationCorrections(
    grpc::CallbackServerContext *context,
    const ULMSAPIV1::StationCorrectionsRequest *request,
    ULMSAPIV1::StationCorrectionsResponse *response)
{
    class Reactor : public grpc::ServerUnaryReactor
    {
    public:
        Reactor(grpc::CallbackServerContext *context,
                const ULMSAPIV1::StationCorrectionsRequest &request,
                ULMSAPIV1::StationCorrectionsResponse *response,
                const std::optional<std::string> &accessToken,
                const Magnitude::NetworkMagnitudeCalculator &calculator,
                std::shared_ptr<spdlog::logger> logger) :
            mLogger(std::move(logger))
        {
            auto &metrics = Metrics::Singleton::getInstance();
            // Validate the client?
            if (accessToken != std::nullopt)
            {
                if (!::validateClient(context, *accessToken))
                {
                    SPDLOG_LOGGER_WARN(mLogger,
                                       "Unauthorized client - {} rejected",
                                       context->peer());
                    metrics.incrementUnauthenticatedCounter(mRouteName);
                    Finish({grpc::StatusCode::UNAUTHENTICATED,
                            "Invalid access token"});
                    return;
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
                            stationCorrection.set_value(*correction);
                            stationCorrection.set_exists(true);
                        }
                        else
                        {
                            stationCorrection.set_value(0);
                            stationCorrection.set_exists(false);
                        }
                        response->mutable_station_corrections()->Add(
                            std::move(stationCorrection));
                    }
                    catch (const std::invalid_argument &e)
                    {
                        metrics.incrementClientErrorCounter(mRouteName);
                        Finish({grpc::StatusCode::INVALID_ARGUMENT,
                                "Malformed station identifier"});
                        return;
                    }
                    catch (const std::exception &e)
                    {
                        metrics.incrementServerErrorCounter(mRouteName);
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
                                    "{} RPC completed", mRouteName);
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
                                    "{} RPC canceled", mRouteName);
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
                       pImpl->mAccessToken,
                       *pImpl->mCalculator,
                       pImpl->mLogger);
}

