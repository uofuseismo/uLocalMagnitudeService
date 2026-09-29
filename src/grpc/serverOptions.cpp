#include <cstdint>
#include <utility>
#include <optional>
#include <string>
#include <stdexcept>
#include <memory>
#include "uLocalMagnitudeService/grpc/serverOptions.hpp"

using namespace ULocalMagnitudeService::GRPC;

class ServerOptions::ServerOptionsImpl
{
public:
    std::string mHost{"localhost"};
    uint16_t mPort{50000};
    std::optional<std::string> mAccessToken;
    std::optional<std::string> mServerCertificate;
    std::optional<std::string> mServerKey;
    std::optional<std::string> mClientCertificate;
    bool mReflectionEnabled{false};
};

/// Constructor
ServerOptions::ServerOptions() :
    pImpl(std::make_unique<ServerOptionsImpl>())
{
}

/// Copy constructor
ServerOptions::ServerOptions(const ServerOptions &options)
{
    *this = options;
}

/// Move constructor
ServerOptions::ServerOptions(ServerOptions &&options) noexcept
{
    *this = std::move(options);
}

/// Copy assignment operator
ServerOptions& ServerOptions::operator=(const ServerOptions &options)
{
    if (&options == this){return *this;}
    pImpl = std::make_unique<ServerOptionsImpl>(*options.pImpl);
    return *this;
}

/// Move assignment
ServerOptions& ServerOptions::operator=(ServerOptions &&options) noexcept
{
    if (this == &options){return *this;}
    pImpl = std::move(options.pImpl);
    return *this;
}

void ServerOptions::setHost(const std::string &host)
{
    if (host.empty()){throw std::invalid_argument("Host cannot be empty.");}
    pImpl->mHost = host;
}

std::string ServerOptions::getHost() const noexcept
{
    return pImpl->mHost;
}

void ServerOptions::setPort(uint16_t port)
{
    if (port == 0){throw std::invalid_argument("Port cannot be 0.");}
    pImpl->mPort = port;
}

uint16_t ServerOptions::getPort() const noexcept
{
    return pImpl->mPort;
}

void ServerOptions::setAccessToken(const std::string &accessToken)
{
    if (accessToken.empty())
    {
        throw std::invalid_argument("Access token cannot be empty.");
    }
    pImpl->mAccessToken = std::optional<std::string> (accessToken);
}

std::optional<std::string> ServerOptions::getAccessToken() const noexcept
{
    return pImpl->mAccessToken;
}

void ServerOptions::setServerCertificate(
    const std::string &serverCertificate)
{
    if (serverCertificate.empty())
    {
        throw std::invalid_argument("Server certificate cannot be empty.");
    }
    pImpl->mServerCertificate = std::optional<std::string> (serverCertificate);
}

std::optional<std::string> 
    ServerOptions::getServerCertificate() const noexcept
{
    return pImpl->mServerCertificate;
}

void ServerOptions::setServerKey(const std::string &serverKey)
{
    if (serverKey.empty())
    {
        throw std::invalid_argument("Server key cannot be empty.");
    }
    pImpl->mServerKey = std::optional<std::string> (serverKey);
}

std::optional<std::string> ServerOptions::getServerKey() const noexcept
{
    return pImpl->mServerKey;
}

void ServerOptions::setClientCertificate(
    const std::string &clientCertificate)
{
    if (clientCertificate.empty())
    {
        throw std::invalid_argument("Client certificate cannot be empty.");
    }
    pImpl->mClientCertificate = std::optional<std::string> (clientCertificate);
}

std::optional<std::string> 
    ServerOptions::getClientCertificate() const noexcept
{
    return pImpl->mClientCertificate;
}

void ServerOptions::enableReflection() noexcept
{
    pImpl->mReflectionEnabled = true;
}

void ServerOptions::disableReflection() noexcept
{
    pImpl->mReflectionEnabled = false;
}

bool ServerOptions::isReflectionEnabled() const noexcept
{
    return pImpl->mReflectionEnabled;
}

/// Check the options make sense in aggregate
void ServerOptions::validate() const
{
    // Server and key are set - everything else is gravy
    if (getServerCertificate() != std::nullopt &&
        getServerKey() != std::nullopt)
    {
        return;
    }
    // After this check we can confirm that no server cert pair is set
    if (getServerCertificate() != std::nullopt ||
        getServerKey() != std::nullopt)
    {
        throw std::runtime_error(
           "Server certificate and server key must both be set for TLS");
    }
    // Token specified but cert pair not set
    if (getAccessToken() != std::nullopt)
    {
        throw std::runtime_error(
           "Access token requires server certs");
    }
    // Client cert specificed but cert pair not set
    if (getClientCertificate() != std::nullopt)
    {
        throw std::runtime_error(
          "Client certificate requires server certs to enable mTLS");
    }
}

ServerOptions::~ServerOptions() = default;
