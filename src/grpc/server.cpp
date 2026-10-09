#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#ifndef NDEBUG
#include <cassert>
#endif
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
//NOLINTNEXTLINE(misc-include-cleaner)
#include <spdlog/sinks/stdout_color_sinks.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
//NOLINTNEXTLINE(misc-include-cleaner)
#include <grpcpp/support/time.h>
#include <grpcpp/impl/channel_argument_option.h>
#include "uLocalMagnitudeService/grpc/server.hpp"
#include "uLocalMagnitudeService/grpc/serverOptions.hpp"
#include "uLocalMagnitudeService/magnitude/networkMagnitudeCalculatorOptions.hpp"
#include "magnitudeService.hpp"

using namespace ULocalMagnitudeService::GRPC;

class Server::ServerImpl
{
public:
    ServerImpl
    (
        const GRPC::ServerOptions &grpcOptions,
        const Magnitude::NetworkMagnitudeCalculatorOptions &calculatorOptions,
        std::shared_ptr<spdlog::logger> logger
    ) :
        mGRPCOptions(grpcOptions),
        mLogger(std::move(logger))
    {
        if (mLogger == nullptr)
        {
            // NOLINTBEGIN(misc-include-cleaner)
            constexpr const char *loggerName{"ServerConsole"};
            mLogger = spdlog::get(loggerName);
            if (mLogger == nullptr)
            {
                mLogger = spdlog::stdout_color_mt(loggerName);
            }
            // NOLINTEND(misc-include-cleaner)
        }
        // Create services
        createServices(calculatorOptions);
    }

    /// Use TLS?
    [[nodiscard]] bool useTLS() const noexcept
    {
        if (mGRPCOptions.getServerKey() == std::nullopt ||
            mGRPCOptions.getServerCertificate() == std::nullopt)
        {
            return false;
        }
        return true;
    }

    /// Run during construction
    void createServices(
        const Magnitude::NetworkMagnitudeCalculatorOptions &calculatorOptions
    )
    {
        SPDLOG_LOGGER_DEBUG(mLogger, "Creating magnitude service");
        // Are we going to secure the server?
        std::optional<std::string> accessToken{std::nullopt};
        if (useTLS()){accessToken = mGRPCOptions.getAccessToken();}
        std::unique_ptr<grpc::Service> magnitudeService
            = std::make_unique<GRPC::MagnitudeService> (
                 calculatorOptions,
                 accessToken,
                 mLogger);
        mServicesMap.insert(
           std::move(
              std::pair {"MagnitudeService", std::move(magnitudeService)}
           )
        );
    }   

    void start()
    {
        grpc::ServerBuilder builder;
        const auto address = mGRPCOptions.getAddress();
        // Add global rules like messages size limits etc.
        builder.SetMaxSendMessageSize(
            mGRPCOptions.getMaximumRequestMessageSizeInBytes());
        builder.SetOption(grpc::MakeChannelArgumentOption(
               "GRPC_ARG_MAX_CONNECTION_AGE_MS",
               static_cast<int>
                  (mGRPCOptions.getMaximumConnectionAge().count())));
        builder.SetOption(grpc::MakeChannelArgumentOption(
               "GRPC_ARG_MAX_CONNECTION_AGE_GRACE_MS",
               static_cast<int>
                  (mGRPCOptions.getMaximumConnectionAgeGracePeriod().count())));
        // Secure the server?
        if (!useTLS())
        {    
            SPDLOG_LOGGER_INFO(mLogger,
                "Initiating non-secured local magnitude service");
            builder.AddListeningPort(address,
                                     grpc::InsecureServerCredentials());
        }
        else
        {
            auto serverKey = mGRPCOptions.getServerKey();
            auto serverCertificate = mGRPCOptions.getServerCertificate();
#ifndef NDEBUG
            assert(serverKey != std::nullopt);
            assert(serverCertificate != std::nullopt);
#endif
            SPDLOG_LOGGER_INFO(mLogger,
                "Initiating secured local magnitude service");
            const grpc::SslServerCredentialsOptions::PemKeyCertPair keyCertPair
            {
                *serverKey,
                *serverCertificate
            };
            grpc::SslServerCredentialsOptions sslOptions;
            sslOptions.pem_key_cert_pairs.emplace_back(keyCertPair);
            builder.AddListeningPort(address,
                                     grpc::SslServerCredentials(sslOptions));
        }
        // Register before start
        for (auto &service : mServicesMap)
        {
            SPDLOG_LOGGER_DEBUG(mLogger, "Registering {}", service.first);
            builder.RegisterService(service.second.get());
        }
        mServer = builder.BuildAndStart();
        if (mServer == nullptr)
        {
            SPDLOG_LOGGER_CRITICAL(mLogger, "Failed to start server");
            throw std::runtime_error("Failed to start server");
        }
        mServerStarted.store(true);
        mServer->Wait();
        mServerStarted.store(false);
    }

    void stop()
    {   
        if (mServer && mServerStarted.exchange(false))
        {
            SPDLOG_LOGGER_INFO(mLogger, "Shutting down service");
            constexpr int64_t timeOutSeconds{1};
            constexpr int64_t timeOutNanoSeconds{0};
            const gpr_timespec deadline // NOLINT
            {
                timeOutSeconds,
                timeOutNanoSeconds,
                GPR_TIMESPAN // NOLINT
            };
            mServer->Shutdown(deadline);
            SPDLOG_LOGGER_INFO(mLogger, "Service shut down");
            mServer = nullptr;
        }
    }   

    ~ServerImpl()
    {   
        stop();
    }   

//private:
    GRPC::ServerOptions mGRPCOptions;
    std::shared_ptr<spdlog::logger> mLogger{nullptr};
    // N.B. Services must outlive mServer (RegisterService keeps only a raw
    //      pointer).  This is guaranteed by declaration order: mServicesMap is
    //      declared before mServer, so it is destroyed after it.
    //      DO NOT reorder this map and the server.
    std::map<std::string, std::unique_ptr<grpc::Service>> mServicesMap;
    std::unique_ptr<grpc::Server> mServer{nullptr};
    std::atomic<bool> mServerStarted{false};
};

/// Constructor
Server::Server(
    const ServerOptions &grpcOptions,
    const Magnitude::NetworkMagnitudeCalculatorOptions &calculatorOptions,
    std::shared_ptr<spdlog::logger> logger
    ) :
    pImpl(std::make_unique<ServerImpl> (grpcOptions, calculatorOptions, std::move(logger)))
{
}

/// Destructor
Server::~Server() = default;
