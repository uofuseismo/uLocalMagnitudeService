#ifndef ULOCAL_MAGNITUDE_SERVICE_GRPC_MAGNITUDE_SERVICE_HPP
#define ULOCAL_MAGNITUDE_SERVICE_GRPC_MAGNITUDE_SERVICE_HPP
#include <memory>
#include <optional>
#include <spdlog/logger.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/server_callback.h>
#include <uLocalMagnitudeServiceAPI/v1/magnitude/service.grpc.pb.h>

namespace ULocalMagnitudeService::Magnitude
{
 class NetworkMagnitudeCalculatorOptions;
}
namespace ULocalMagnitudeServiceAPI::V1::Magnitude
{
 class NetworkMagnitudeFromAmplitudesRequest;
 class NetworkMagnitudeFromAmplitudesResponse;
 class StationCorrectionsRequest;
 class StationCorrectionsResponse;
 class StationMagnitudesFromAmplitudesRequest;
 class StationMagnitudesFromAmplitudesResponse;
}
namespace ULocalMagnitudeService::GRPC
{
 class MagnitudeServiceOptions;
}

namespace ULocalMagnitudeService::GRPC
{
/// @brief Implements the gRPC magnitudes from amplitudes service.
class MagnitudeService final : public
    ULocalMagnitudeServiceAPI::V1::Magnitude::MagnitudeService::CallbackService
{
public:
    /// @brief Constructor. 
    MagnitudeService(
        const ULocalMagnitudeService::Magnitude::NetworkMagnitudeCalculatorOptions &calculatorOptions,
        std::optional<std::string> &accessToken,
        std::shared_ptr<spdlog::logger> logger);
 
    /// @brief Computes the network magnitude from amplitudes.
    grpc::ServerUnaryReactor
        *ComputeNetworkMagnitudeFromAmplitudes(
            grpc::CallbackServerContext *context,
            const ULocalMagnitudeServiceAPI::V1::Magnitude::NetworkMagnitudeFromAmplitudesRequest *request,
            ULocalMagnitudeServiceAPI::V1::Magnitude::NetworkMagnitudeFromAmplitudesResponse *response) override;

    /// @brief Computes the station magnitudes from amplitudes. 
    grpc::ServerUnaryReactor
        *ComputeStationMagnitudesFromAmplitudes(
            grpc::CallbackServerContext *context,
            const ULocalMagnitudeServiceAPI::V1::Magnitude::StationMagnitudesFromAmplitudesRequest *request,
            ULocalMagnitudeServiceAPI::V1::Magnitude::StationMagnitudesFromAmplitudesResponse *response) override;

    /// @brief Gets station corrections for debugging.
    grpc::ServerUnaryReactor
        *GetStationCorrections(
            grpc::CallbackServerContext *context,
            const ULocalMagnitudeServiceAPI::V1::Magnitude::StationCorrectionsRequest *request,
            ULocalMagnitudeServiceAPI::V1::Magnitude::StationCorrectionsResponse *response) override;

    /// @brief Destructor.
    ~MagnitudeService();
 
    MagnitudeService() = delete;
    MagnitudeService(const MagnitudeService &) = delete;
    MagnitudeService(MagnitudeService &&) noexcept = delete;
    MagnitudeService &operator=(const MagnitudeService &) = delete;
    MagnitudeService &operator=(MagnitudeService &&) noexcept = delete;
private:
    class MagnitudeServiceImpl;
    std::unique_ptr<MagnitudeServiceImpl> pImpl;
};
}
#endif
