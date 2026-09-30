#ifndef ULOCAL_MAGNITUDE_SERVICE_GRPC_MAGNITUDE_SERVICE_HPP
#define ULOCAL_MAGNITUDE_SERVICE_GRPC_MAGNITUDE_SERVICE_HPP
#include <memory>
#include <spdlog/logger.h>

namespace ULocalMagnitudeService::Magnitude
{
 class NetworkOptions;
}
namespace ULocalMagnitudeService::GRPC
{
 class MagnitudeServiceOptions;
}

namespace ULocalMagnitudeService::GRPC
{
class MagnitudeService
{
public:
    
    MagnitudeService(std::shared_ptr<spdlog::logger> logger);
    /// @brief Destructor.
    ~MagnitudeService();
 
    MagnitudeService() = delete;
    MagnitudeService(const MagnitudeService &) = delete;
    MagnitudeService(MagnitudeService &&) noexcept = delete;
    MagnitudeService &operator=(const MagnitudeService &) = delete;
    MagnitudeService &oeprator=(MagnitudeService &&) noexcept = delete;
private:
    class MagnitudeServiceImpl;
    std::unique_ptr<MagnitudeServiceImpl> pImpl;
};
}
#endif
