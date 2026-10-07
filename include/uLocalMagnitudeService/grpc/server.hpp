#ifndef ULOCAL_MAGNITUDE_SERVICE_GRPC_SERVER_HPP
#define ULOCAL_MAGNITUDE_SERVICE_GRPC_SERVER_HPP
#include <memory>
#include <spdlog/logger.h>
namespace ULocalMagnitudeService::GRPC
{
 class ServerOptions;
}
namespace ULocalMagnitudeService::GRPC
{
/// @class Server server.hpp
/// @brief This is the server that manage the services with which the clients
///        interact.  
/// @copyright Ben Baker (University of Utah) distributed under the
///            MIT NO AI license.
class Server
{
public:
    Server(std::shared_ptr<spdlog::logger> logger);

    /// @brief Destructor.
    ~Server();

    Server& operator=(const Server &) = delete;
    Server& operator=(Server &&) noexcept = delete;
    Server(const Server &) = delete;
    Server(Server &&) noexcept = delete;
private:
    class ServerImpl;
    std::unique_ptr<ServerImpl> pImpl;
};
}
#endif
