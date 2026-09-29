#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include "uLocalMagnitudeService/grpc/serverOptions.hpp"

using namespace ULocalMagnitudeService::GRPC;

namespace
{
/// Something that looks like a PEM without being one.
const std::string serverCertificate{
    "-----BEGIN CERTIFICATE-----\nserver\n-----END CERTIFICATE-----\n"};
const std::string serverKey{
    "-----BEGIN PRIVATE KEY-----\nkey\n-----END PRIVATE KEY-----\n"};
const std::string clientCertificate{
    "-----BEGIN CERTIFICATE-----\nclient\n-----END CERTIFICATE-----\n"};
const std::string accessToken{"abc123"};

/// Sets every option to a non-default value.
ServerOptions makeOptions()
{
    ServerOptions options;
    options.setHost("0.0.0.0");
    options.setPort(8443);
    options.setAccessToken(accessToken);
    options.setServerCertificate(serverCertificate);
    options.setServerKey(serverKey);
    options.setClientCertificate(clientCertificate);
    options.enableReflection();
    return options;
}

void checkOptions(const ServerOptions &options)
{
    REQUIRE(options.getHost() == "0.0.0.0");
    REQUIRE(options.getPort() == 8443);
    REQUIRE(options.getAccessToken() == accessToken);
    REQUIRE(options.getServerCertificate() == serverCertificate);
    REQUIRE(options.getServerKey() == serverKey);
    REQUIRE(options.getClientCertificate() == clientCertificate);
    REQUIRE(options.isReflectionEnabled());
    REQUIRE_NOTHROW(options.validate());
}
}

TEST_CASE("ULocalMagnitudeService::GRPC::ServerOptions", "[serverOptions]")
{
    SECTION("Defaults")
    {
        const ServerOptions options;
        REQUIRE(options.getHost() == "localhost");
        REQUIRE(options.getPort() == 50000);
        REQUIRE_FALSE(options.getAccessToken().has_value());
        REQUIRE_FALSE(options.getServerCertificate().has_value());
        REQUIRE_FALSE(options.getServerKey().has_value());
        REQUIRE_FALSE(options.getClientCertificate().has_value());
        REQUIRE_FALSE(options.isReflectionEnabled());
        // No TLS and no token is a valid (insecure) server
        REQUIRE_NOTHROW(options.validate());
    }

    SECTION("Host")
    {
        ServerOptions options;
        options.setHost("0.0.0.0");
        REQUIRE(options.getHost() == "0.0.0.0");
        options.setHost("magnitude.seis.utah.edu");
        REQUIRE(options.getHost() == "magnitude.seis.utah.edu");
        options.setHost("[::1]");
        REQUIRE(options.getHost() == "[::1]");
        // An empty host is rejected and the previous one is kept
        REQUIRE_THROWS_AS(options.setHost(""), std::invalid_argument);
        REQUIRE(options.getHost() == "[::1]");
    }

    SECTION("Port")
    {
        ServerOptions options;
        options.setPort(1);
        REQUIRE(options.getPort() == 1);
        options.setPort(UINT16_MAX);
        REQUIRE(options.getPort() == 65535);
        // Port 0 (let the OS pick) is rejected and the previous one is kept
        REQUIRE_THROWS_AS(options.setPort(0), std::invalid_argument);
        REQUIRE(options.getPort() == 65535);
    }

    SECTION("Access token")
    {
        ServerOptions options;
        options.setAccessToken(accessToken);
        REQUIRE(options.getAccessToken() == accessToken);
        options.setAccessToken("def456");
        REQUIRE(options.getAccessToken() == "def456");
        REQUIRE_THROWS_AS(options.setAccessToken(""), std::invalid_argument);
        REQUIRE(options.getAccessToken() == "def456");
    }

    SECTION("Server certificate")
    {
        ServerOptions options;
        options.setServerCertificate(serverCertificate);
        REQUIRE(options.getServerCertificate() == serverCertificate);
        REQUIRE_THROWS_AS(options.setServerCertificate(""),
                          std::invalid_argument);
        REQUIRE(options.getServerCertificate() == serverCertificate);
        // Nothing else is touched
        REQUIRE_FALSE(options.getServerKey().has_value());
        REQUIRE_FALSE(options.getClientCertificate().has_value());
    }

    SECTION("Server key")
    {
        ServerOptions options;
        options.setServerKey(serverKey);
        REQUIRE(options.getServerKey() == serverKey);
        REQUIRE_THROWS_AS(options.setServerKey(""), std::invalid_argument);
        REQUIRE(options.getServerKey() == serverKey);
        REQUIRE_FALSE(options.getServerCertificate().has_value());
        REQUIRE_FALSE(options.getClientCertificate().has_value());
    }

    SECTION("Client certificate")
    {
        ServerOptions options;
        options.setClientCertificate(clientCertificate);
        REQUIRE(options.getClientCertificate() == clientCertificate);
        REQUIRE_THROWS_AS(options.setClientCertificate(""),
                          std::invalid_argument);
        REQUIRE(options.getClientCertificate() == clientCertificate);
        REQUIRE_FALSE(options.getServerCertificate().has_value());
        REQUIRE_FALSE(options.getServerKey().has_value());
    }

    SECTION("Reflection")
    {
        ServerOptions options;
        options.enableReflection();
        REQUIRE(options.isReflectionEnabled());
        options.enableReflection();
        REQUIRE(options.isReflectionEnabled());
        options.disableReflection();
        REQUIRE_FALSE(options.isReflectionEnabled());
        options.disableReflection();
        REQUIRE_FALSE(options.isReflectionEnabled());
    }

    SECTION("Validate")
    {
        // Plain server
        const ServerOptions insecure;
        REQUIRE_NOTHROW(insecure.validate());

        // TLS needs both the certificate and the key
        ServerOptions certificateOnly;
        certificateOnly.setServerCertificate(serverCertificate);
        REQUIRE_THROWS_AS(certificateOnly.validate(), std::runtime_error);

        ServerOptions keyOnly;
        keyOnly.setServerKey(serverKey);
        REQUIRE_THROWS_AS(keyOnly.validate(), std::runtime_error);

        ServerOptions tls;
        tls.setServerCertificate(serverCertificate);
        tls.setServerKey(serverKey);
        REQUIRE_NOTHROW(tls.validate());

        // An access token needs TLS
        ServerOptions tokenOnly;
        tokenOnly.setAccessToken(accessToken);
        REQUIRE_THROWS_AS(tokenOnly.validate(), std::runtime_error);

        ServerOptions tokenAndCertificate;
        tokenAndCertificate.setAccessToken(accessToken);
        tokenAndCertificate.setServerCertificate(serverCertificate);
        REQUIRE_THROWS_AS(tokenAndCertificate.validate(), std::runtime_error);

        ServerOptions tokenAndKey;
        tokenAndKey.setAccessToken(accessToken);
        tokenAndKey.setServerKey(serverKey);
        REQUIRE_THROWS_AS(tokenAndKey.validate(), std::runtime_error);

        ServerOptions tlsWithToken{tls};
        tlsWithToken.setAccessToken(accessToken);
        REQUIRE_NOTHROW(tlsWithToken.validate());

        // A client certificate (mTLS) needs TLS
        ServerOptions clientCertificateOnly;
        clientCertificateOnly.setClientCertificate(clientCertificate);
        REQUIRE_THROWS_AS(clientCertificateOnly.validate(),
                          std::runtime_error);

        ServerOptions clientCertificateAndKey;
        clientCertificateAndKey.setClientCertificate(clientCertificate);
        clientCertificateAndKey.setServerKey(serverKey);
        REQUIRE_THROWS_AS(clientCertificateAndKey.validate(),
                          std::runtime_error);

        ServerOptions mTLS{tls};
        mTLS.setClientCertificate(clientCertificate);
        REQUIRE_NOTHROW(mTLS.validate());

        ServerOptions mTLSWithToken{mTLS};
        mTLSWithToken.setAccessToken(accessToken);
        REQUIRE_NOTHROW(mTLSWithToken.validate());

        // Order doesn't matter - it's the final state that counts
        ServerOptions tokenFirst;
        tokenFirst.setAccessToken(accessToken);
        tokenFirst.setClientCertificate(clientCertificate);
        REQUIRE_THROWS_AS(tokenFirst.validate(), std::runtime_error);
        tokenFirst.setServerKey(serverKey);
        REQUIRE_THROWS_AS(tokenFirst.validate(), std::runtime_error);
        tokenFirst.setServerCertificate(serverCertificate);
        REQUIRE_NOTHROW(tokenFirst.validate());

        // Host, port, and reflection don't factor in
        ServerOptions everythingElse;
        everythingElse.setHost("0.0.0.0");
        everythingElse.setPort(8443);
        everythingElse.enableReflection();
        REQUIRE_NOTHROW(everythingElse.validate());
    }

    SECTION("Copy and move")
    {
        auto options = makeOptions();

        // Copy constructor
        const ServerOptions copy{options};
        checkOptions(copy);

        // Copy is deep: modifying the original doesn't touch the copy
        options.setHost("localhost");
        options.setPort(50000);
        options.setAccessToken("def456");
        options.disableReflection();
        checkOptions(copy);

        // Copy assignment
        ServerOptions copyAssigned;
        copyAssigned = copy;
        checkOptions(copyAssigned);

        // Move constructor
        ServerOptions moved{std::move(copyAssigned)};
        checkOptions(moved);

        // Move assignment
        ServerOptions moveAssigned;
        moveAssigned = std::move(moved);
        checkOptions(moveAssigned);
    }
}
