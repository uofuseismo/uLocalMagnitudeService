#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <random>
#include <stdexcept>
#include <string>
#include <system_error>
#include <catch2/catch_test_macros.hpp>
#include "uLocalMagnitudeService/grpc/serverOptions.hpp"

using namespace ULocalMagnitudeService::GRPC;

namespace
{
/// Writes a file to the temporary directory and removes it when it goes out
/// of scope.  ctest runs test cases in parallel processes so the name gets a
/// random suffix (ahead of the extension) to keep tests from deleting each
/// other's files.
class TemporaryFile
{
public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    TemporaryFile(const std::string &name, const std::string &contents) :
        mPath(std::filesystem::temp_directory_path()
            / ("uLocalMagnitudeService_serverOptions_"
             + std::filesystem::path {name}.stem().string() + "_"
             + std::format("{:016x}", randomSuffix())
             + std::filesystem::path {name}.extension().string()))
    {
        std::ofstream file(mPath);
        file << contents;
    }
    ~TemporaryFile()
    {
        std::error_code errorCode;
        std::filesystem::remove(mPath, errorCode);
    }
    TemporaryFile(const TemporaryFile &) = delete;
    TemporaryFile& operator=(const TemporaryFile &) = delete;
    [[nodiscard]] const std::filesystem::path &path() const noexcept
    {
        return mPath;
    }
private:
    static std::uint64_t randomSuffix()
    {
        std::random_device device;
        return (static_cast<std::uint64_t> (device()) << 32U) | device();
    }
    std::filesystem::path mPath;
};

const std::string serverCertificate{
    "-----BEGIN CERTIFICATE-----\nserver\n-----END CERTIFICATE-----"};
const std::string serverKey{
    "-----BEGIN PRIVATE KEY-----\nkey\n-----END PRIVATE KEY-----"};
const std::string clientCertificate{
    "-----BEGIN CERTIFICATE-----\nclient\n-----END CERTIFICATE-----"};
const std::string accessToken{"abc123"};
}

TEST_CASE("ULocalMagnitudeService::GRPC::fromInitializationFile",
          "[serverOptions]")
{
    // The PEMs are multi-line so they have to come from files.  A trailing
    // newline is what a shell or editor typically leaves behind.
    const TemporaryFile serverCertificateFile("server.crt",
                                              serverCertificate + "\n");
    const TemporaryFile serverKeyFile("server.key", serverKey + "\n");
    const TemporaryFile clientCertificateFile("client.crt",
                                              clientCertificate + "\n");
    const TemporaryFile accessTokenFile("token.txt", accessToken + "\n");
    const auto tlsSettings
        = "serverCertificateFile = " + serverCertificateFile.path().string()
        + "\nserverKeyFile = " + serverKeyFile.path().string() + "\n";

    SECTION("Missing file")
    {
        REQUIRE_THROWS_AS(
            fromInitializationFile("/this/file/does/not/exist.ini"),
            std::invalid_argument);
    }

    SECTION("Empty section takes the defaults")
    {
        const TemporaryFile iniFile("defaults.ini", "[GRPCServer]\n");
        const auto options = fromInitializationFile(iniFile.path());
        REQUIRE(options.getHost() == "localhost");
        REQUIRE(options.getPort() == 50000);
        REQUIRE_FALSE(options.getServerCertificate().has_value());
        REQUIRE_FALSE(options.getServerKey().has_value());
        REQUIRE_FALSE(options.getAccessToken().has_value());
        REQUIRE_FALSE(options.getClientCertificate().has_value());
        REQUIRE(options.getMaximumRequestMessageSizeInBytes() == 65536);
        REQUIRE(options.getMaximumConnectionAge() == std::chrono::minutes {2});
        REQUIRE(options.getMaximumConnectionAgeGracePeriod()
             == std::chrono::seconds {2});
        REQUIRE(options.getAddress() == "localhost:50000");
    }

    SECTION("Host and port")
    {
        const TemporaryFile iniFile("hostPort.ini",
                                    "[GRPCServer]\n"
                                    "host = 0.0.0.0\n"
                                    "port = 8443\n");
        const auto options = fromInitializationFile(iniFile.path());
        REQUIRE(options.getHost() == "0.0.0.0");
        REQUIRE(options.getPort() == 8443);
        REQUIRE(options.getAddress() == "0.0.0.0:8443");
    }

    SECTION("Custom section")
    {
        const TemporaryFile iniFile("customSection.ini",
                                    "[GRPCServer]\n"
                                    "port = 8443\n"
                                    "[MyServer]\n"
                                    "host = magnitude.seis.utah.edu\n"
                                    "port = 9000\n");
        const auto options
            = fromInitializationFile(iniFile.path(), "MyServer");
        REQUIRE(options.getHost() == "magnitude.seis.utah.edu");
        REQUIRE(options.getPort() == 9000);
        // A trailing dot on the section name isn't doubled up
        const auto dotted
            = fromInitializationFile(iniFile.path(), "MyServer.");
        REQUIRE(dotted.getAddress() == "magnitude.seis.utah.edu:9000");
    }

    SECTION("Maximum request message size")
    {
        const TemporaryFile iniFile("maxMessage.ini",
                                    "[GRPCServer]\n"
                                    "maximumRequestMessageSizeInBytes = 1048576\n");
        REQUIRE(fromInitializationFile(iniFile.path())
                .getMaximumRequestMessageSizeInBytes() == 1048576);

        const TemporaryFile zero("maxMessageZero.ini",
                                 "[GRPCServer]\n"
                                 "maximumRequestMessageSizeInBytes = 0\n");
        REQUIRE_THROWS_AS(fromInitializationFile(zero.path()),
                          std::invalid_argument);

        const TemporaryFile negative("maxMessageNegative.ini",
                                     "[GRPCServer]\n"
                                     "maximumRequestMessageSizeInBytes = -10\n");
        REQUIRE_THROWS_AS(fromInitializationFile(negative.path()),
                          std::invalid_argument);

        const TemporaryFile notANumber("maxMessageText.ini",
                                       "[GRPCServer]\n"
                                       "maximumRequestMessageSizeInBytes = 4MB\n");
        REQUIRE_THROWS(fromInitializationFile(notANumber.path()));
    }

    SECTION("Maximum connection age and grace period")
    {
        const TemporaryFile iniFile(
            "connectionAge.ini",
            "[GRPCServer]\n"
            "maximumConnectionAgeInMilliSeconds = 600000\n"
            "maximumConnectionAgeGracePeriodInMilliSeconds = 30000\n");
        const auto options = fromInitializationFile(iniFile.path());
        REQUIRE(options.getMaximumConnectionAge() == std::chrono::minutes {10});
        REQUIRE(options.getMaximumConnectionAgeGracePeriod()
             == std::chrono::seconds {30});

        const TemporaryFile noGrace(
            "noGracePeriod.ini",
            "[GRPCServer]\n"
            "maximumConnectionAgeGracePeriodInMilliSeconds = 0\n");
        REQUIRE(fromInitializationFile(noGrace.path())
                .getMaximumConnectionAgeGracePeriod()
             == std::chrono::milliseconds {0});

        const TemporaryFile zeroAge(
            "zeroConnectionAge.ini",
            "[GRPCServer]\n"
            "maximumConnectionAgeInMilliSeconds = 0\n");
        REQUIRE_THROWS_AS(fromInitializationFile(zeroAge.path()),
                          std::invalid_argument);

        const TemporaryFile negativeGrace(
            "negativeGracePeriod.ini",
            "[GRPCServer]\n"
            "maximumConnectionAgeGracePeriodInMilliSeconds = -1\n");
        REQUIRE_THROWS_AS(fromInitializationFile(negativeGrace.path()),
                          std::invalid_argument);
    }

    SECTION("Reflection")
    {
        const TemporaryFile enabled("reflectionOn.ini",
                                    "[GRPCServer]\n"
                                    "enableReflection = true\n");
        REQUIRE(fromInitializationFile(enabled.path()).isReflectionEnabled());
        const TemporaryFile disabled("reflectionOff.ini",
                                     "[GRPCServer]\n"
                                     "enableReflection = false\n");
        REQUIRE_FALSE(
            fromInitializationFile(disabled.path()).isReflectionEnabled());
        const TemporaryFile unset("reflectionUnset.ini", "[GRPCServer]\n");
        REQUIRE_FALSE(
            fromInitializationFile(unset.path()).isReflectionEnabled());
    }

    SECTION("Invalid host and port")
    {
        const TemporaryFile emptyHost("emptyHost.ini",
                                      "[GRPCServer]\n"
                                      "host =\n");
        REQUIRE_THROWS_AS(fromInitializationFile(emptyHost.path()),
                          std::invalid_argument);
        const TemporaryFile zeroPort("zeroPort.ini",
                                     "[GRPCServer]\n"
                                     "port = 0\n");
        REQUIRE_THROWS_AS(fromInitializationFile(zeroPort.path()),
                          std::invalid_argument);
    }

    SECTION("TLS from files")
    {
        const TemporaryFile iniFile("tls.ini", "[GRPCServer]\n" + tlsSettings);
        const auto options = fromInitializationFile(iniFile.path());
        REQUIRE(options.getServerCertificate() == serverCertificate);
        REQUIRE(options.getServerKey() == serverKey);
        REQUIRE_FALSE(options.getAccessToken().has_value());
        REQUIRE_FALSE(options.getClientCertificate().has_value());
    }

    SECTION("TLS with an access token")
    {
        const TemporaryFile inlineToken("inlineToken.ini",
                                        "[GRPCServer]\n" + tlsSettings
                                      + "accessToken = " + accessToken + "\n");
        REQUIRE(fromInitializationFile(inlineToken.path()).getAccessToken()
                == accessToken);

        // The trailing newline in the file is trimmed
        const TemporaryFile fileToken("fileToken.ini",
                                      "[GRPCServer]\n" + tlsSettings
                                    + "accessTokenFile = "
                                    + accessTokenFile.path().string() + "\n");
        const auto options = fromInitializationFile(fileToken.path());
        REQUIRE(options.getAccessToken() == accessToken);
        // The token doesn't leak into the client certificate
        REQUIRE_FALSE(options.getClientCertificate().has_value());
    }

    SECTION("TLS with a client certificate")
    {
        const TemporaryFile iniFile("mTLS.ini",
                                    "[GRPCServer]\n" + tlsSettings
                                  + "clientCertificateFile = "
                                  + clientCertificateFile.path().string()
                                  + "\n");
        const auto options = fromInitializationFile(iniFile.path());
        REQUIRE(options.getClientCertificate() == clientCertificate);
        REQUIRE_FALSE(options.getAccessToken().has_value());
    }

    SECTION("Inline secrets use the documented keys")
    {
        // Single-line stand-ins - the reader doesn't parse PEMs
        const TemporaryFile iniFile("inlineSecrets.ini",
                                    "[GRPCServer]\n"
                                    "serverCertificate = server-certificate\n"
                                    "serverKey = server-key\n"
                                    "accessToken = " + accessToken + "\n"
                                    "clientCertificate = client-certificate\n");
        const auto options = fromInitializationFile(iniFile.path());
        REQUIRE(options.getServerCertificate() == "server-certificate");
        REQUIRE(options.getServerKey() == "server-key");
        REQUIRE(options.getAccessToken() == accessToken);
        REQUIRE(options.getClientCertificate() == "client-certificate");
    }

    SECTION("The client certificate isn't read from the token keys")
    {
        // The reader used to look for the client certificate under
        // clientToken/clientTokenFile
        const TemporaryFile iniFile("clientToken.ini",
                                    "[GRPCServer]\n" + tlsSettings
                                  + "clientTokenFile = "
                                  + clientCertificateFile.path().string()
                                  + "\n");
        const auto options = fromInitializationFile(iniFile.path());
        REQUIRE_FALSE(options.getClientCertificate().has_value());
        REQUIRE_FALSE(options.getAccessToken().has_value());
    }

    SECTION("Everything")
    {
        const TemporaryFile iniFile("everything.ini",
                                    "[GRPCServer]\n"
                                    "host = 0.0.0.0\n"
                                    "port = 8443\n"
                                    "enableReflection = true\n"
                                    "maximumRequestMessageSizeInBytes = 1048576\n"
                                    "maximumConnectionAgeInMilliSeconds = 600000\n"
                                    "maximumConnectionAgeGracePeriodInMilliSeconds = 30000\n"
                                  + tlsSettings
                                  + "accessTokenFile = "
                                  + accessTokenFile.path().string() + "\n"
                                  + "clientCertificateFile = "
                                  + clientCertificateFile.path().string()
                                  + "\n");
        const auto options = fromInitializationFile(iniFile.path());
        REQUIRE(options.getHost() == "0.0.0.0");
        REQUIRE(options.getPort() == 8443);
        REQUIRE(options.getServerCertificate() == serverCertificate);
        REQUIRE(options.getServerKey() == serverKey);
        REQUIRE(options.getAccessToken() == accessToken);
        REQUIRE(options.getClientCertificate() == clientCertificate);
        REQUIRE(options.isReflectionEnabled());
        REQUIRE(options.getMaximumRequestMessageSizeInBytes() == 1048576);
        REQUIRE(options.getMaximumConnectionAge() == std::chrono::minutes {10});
        REQUIRE(options.getMaximumConnectionAgeGracePeriod()
             == std::chrono::seconds {30});
        REQUIRE(options.getAddress() == "0.0.0.0:8443");
    }

    SECTION("Half of the TLS pair is rejected")
    {
        const TemporaryFile certificateOnly(
            "certificateOnly.ini",
            "[GRPCServer]\nserverCertificateFile = "
          + serverCertificateFile.path().string() + "\n");
        REQUIRE_THROWS_AS(fromInitializationFile(certificateOnly.path()),
                          std::invalid_argument);
        const TemporaryFile keyOnly(
            "keyOnly.ini",
            "[GRPCServer]\nserverKeyFile = "
          + serverKeyFile.path().string() + "\n");
        REQUIRE_THROWS_AS(fromInitializationFile(keyOnly.path()),
                          std::invalid_argument);
    }

    SECTION("Access token or client certificate without TLS is rejected")
    {
        const TemporaryFile token("tokenNoTLS.ini",
                                  "[GRPCServer]\naccessToken = "
                                + accessToken + "\n");
        REQUIRE_THROWS_AS(fromInitializationFile(token.path()),
                          std::invalid_argument);
        const TemporaryFile client("clientNoTLS.ini",
                                   "[GRPCServer]\nclientCertificateFile = "
                                 + clientCertificateFile.path().string()
                                 + "\n");
        REQUIRE_THROWS_AS(fromInitializationFile(client.path()),
                          std::invalid_argument);
    }

    SECTION("A setting given inline and as a file is rejected")
    {
        const TemporaryFile iniFile("both.ini",
                                    "[GRPCServer]\n" + tlsSettings
                                  + "accessToken = " + accessToken + "\n"
                                  + "accessTokenFile = "
                                  + accessTokenFile.path().string() + "\n");
        REQUIRE_THROWS_AS(fromInitializationFile(iniFile.path()),
                          std::invalid_argument);
    }

    SECTION("Missing or empty secret files are rejected")
    {
        const TemporaryFile missing("missingSecret.ini",
                                    "[GRPCServer]\n"
                                    "serverCertificateFile = /does/not/exist.crt\n"
                                    "serverKeyFile = "
                                  + serverKeyFile.path().string() + "\n");
        REQUIRE_THROWS_AS(fromInitializationFile(missing.path()),
                          std::invalid_argument);
        const TemporaryFile blank("blank.key", " \n\t\n");
        const TemporaryFile empty("emptySecret.ini",
                                  "[GRPCServer]\n"
                                  "serverCertificateFile = "
                                + serverCertificateFile.path().string() + "\n"
                                + "serverKeyFile = "
                                + blank.path().string() + "\n");
        REQUIRE_THROWS_AS(fromInitializationFile(empty.path()),
                          std::invalid_argument);
    }
}
