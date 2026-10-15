#ifndef ULOCAL_MAGNITUDE_SERVICE_GRPC_MAGNITUDE_SERVICE_HPP
#define ULOCAL_MAGNITUDE_SERVICE_GRPC_MAGNITUDE_SERVICE_HPP
#include <memory>
#include <optional>
#include <spdlog/logger.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/server_callback.h>
#include <uLocalMagnitudeServiceAPI/magnitude/v1/service.grpc.pb.h>

namespace ULocalMagnitudeService::Magnitude
{
 class NetworkMagnitudeCalculatorOptions;
}
namespace ULocalMagnitudeServiceAPI::Magnitude::V1
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
    ULocalMagnitudeServiceAPI::Magnitude::V1::MagnitudeService::CallbackService
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
            const ULocalMagnitudeServiceAPI::Magnitude::V1::NetworkMagnitudeFromAmplitudesRequest *request,
            ULocalMagnitudeServiceAPI::Magnitude::V1::NetworkMagnitudeFromAmplitudesResponse *response) override;

    /// @brief Computes the station magnitudes from amplitudes. 
    grpc::ServerUnaryReactor
        *ComputeStationMagnitudesFromAmplitudes(
            grpc::CallbackServerContext *context,
            const ULocalMagnitudeServiceAPI::Magnitude::V1::StationMagnitudesFromAmplitudesRequest *request,
            ULocalMagnitudeServiceAPI::Magnitude::V1::StationMagnitudesFromAmplitudesResponse *response) override;

    /// @brief Gets station corrections for debugging.
    grpc::ServerUnaryReactor
        *GetStationCorrections(
            grpc::CallbackServerContext *context,
            const ULocalMagnitudeServiceAPI::Magnitude::V1::StationCorrectionsRequest *request,
            ULocalMagnitudeServiceAPI::Magnitude::V1::StationCorrectionsResponse *response) override;

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
