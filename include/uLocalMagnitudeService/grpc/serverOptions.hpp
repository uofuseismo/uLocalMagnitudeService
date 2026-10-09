#ifndef ULOCAL_MAGNITUDE_SERVICE_GRPC_SERVER_OPTIONS_HPP
#define ULOCAL_MAGNITUDE_SERVICE_GRPC_SERVER_OPTIONS_HPP
#include <cstdint>
#include <filesystem>
#include <string>
#include <optional>
#include <memory>
namespace ULocalMagnitudeService::GRPC
{
/// @class ServerOptions serverOptions.hpp
/// @brief This class is used to configure the gRPC server options.
/// @copyright Ben Baker (University of Utah) distributed under the
///             MIT NO AI license.
class ServerOptions
{
public:
    /// @brief Constructor.
    ServerOptions();
    /// @brief Copy constructor.
    ServerOptions(const ServerOptions &options);
    /// @brief Move constructor.
    ServerOptions(ServerOptions &&options) noexcept;

    /// @brief Sets the host.
    /// @param host The host to set.
    /// @throws std::invalid_argument if the host is empty.
    void setHost(const std::string &host);
    /// @brief Gets the host.
    /// @return The host.
    /// @note By default this is localhost.
    [[nodiscard]] std::string getHost() const noexcept;

    /// @brief Sets the port.
    /// @param port The port to set.
    /// @throws std::invalid_argument if the port is 0.
    void setPort(uint16_t port);
    /// @brief Gets the port.
    /// @return The port.
    /// @note By default this is 50000.
    [[nodiscard]] uint16_t getPort() const noexcept;

    /// @result Combines the hot and port into an address like host:port
    [[nodiscard]] std::string getAddress() const noexcept;

    /// @brief Sets the access token.
    /// @param accessToken The access token to set.
    /// @note Access tokens can only be used by gRPC if the server certificate
    ///       and server key are set.
    /// @throws std::invalid_argument if the token is empty.
    void setAccessToken(const std::string &accessToken);
    /// @brief Gets the access token.
    /// @return The access token.  If this is not set, then std::nullopt is returned.
    [[nodiscard]] std::optional<std::string> getAccessToken() const noexcept;

    /// @brief Sets the server certificate.
    /// @param serverCertificate  The server certificate to set.
    /// @throws std::invalid_argument if the server certificate is empty.
    void setServerCertificate(const std::string &serverCertificate);
    /// @brief Gets the server certificate.
    /// @return The server certificate.  If this is not set, then std::nullopt is returned.
    [[nodiscard]] std::optional<std::string> getServerCertificate() const noexcept;

    /// @brief Sets the server's private key.
    /// @param serverKey The server key to set.
    /// @throws std::invalid_argument if the server's private key is empty.
    void setServerKey(const std::string &serverKey);
    /// @brief Gets the server's private key.
    /// @return The server's private key.  If this is not set, then std::nullopt is returned.
    [[nodiscard]] std::optional<std::string> getServerKey() const noexcept;

    /// @brief Sets the client certificate.
    /// @param clientCertificate  The client certificate to set.
    /// @throws std::invalid_argument if the client's certificate is empty.
    void setClientCertificate(const std::string &clientCertificate);
    /// @brief Gets the client certificate.
    /// @return The client certificate.  If this is not set, then std::nullopt is returned.
    [[nodiscard]] std::optional<std::string> getClientCertificate() const noexcept;

    /// @brief Enables reflection.
    void enableReflection() noexcept;
    /// @brief Disables reflection.
    void disableReflection() noexcept;
    /// @brief Gets whether reflection is enabled.
    /// @return True if reflection is enabled, false otherwise.  By default this is false.
    [[nodiscard]] bool isReflectionEnabled() const noexcept;

    /// @brief Sets the maximum request message size in bytes.
    /// @throws std::invalid_argumetn if maxMessageSize is not positive.
    void setMaximumRequestMessageSizeInBytes(int maxMessageSize);
    /// @result The maximum message size in bytes.  By default this is 4096.
    [[nodiscard]] int getMaximumRequestMessageSizeInBytes() const noexcept;

    /// @brief Checks that the options make sense in aggregate.  The
    ///        individual setters only check each option on its own.
    /// @throws std::runtime_error if only one of the server certificate and
    ///         server key is set, or if an access token or client certificate
    ///         is set without the server certificate and server key.
    void validate() const;

    /// @brief Destructor.
    ~ServerOptions();
    /// @brief Copy assignment operator.
    ServerOptions& operator=(const ServerOptions &options);
    /// @brief Move assignment operator.
    ServerOptions& operator=(ServerOptions &&options) noexcept;
private:
    class ServerOptionsImpl;
    std::unique_ptr<ServerOptionsImpl> pImpl;
};
/// @brief Loads the gRPC server options from an initialization file.
/// @param[in] iniFile  The path to the initialization file.
/// @param[in] section  The section of the initialization file to read.
/// @throws std::invalid_argument if the initialization file does not exist
///         or any parameters are wrong. 
[[nodiscard]] ServerOptions fromInitializationFile(
    const std::filesystem::path &iniFile,
    const std::string &section = "GRPCServer");

}
#endif
