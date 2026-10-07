#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <utility>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
//NOLINTNEXTLINE(misc-include-cleaner)
#include <spdlog/sinks/stdout_color_sinks.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/security/server_credentials.h>
//NOLINTNEXTLINE(misc-include-cleaner)
#include <grpcpp/support/time.h>
#include "uLocalMagnitudeService/grpc/server.hpp"
#include "uLocalMagnitudeService/grpc/serverOptions.hpp"
#include "uLocalMagnitudeService/magnitude/networkMagnitudeCalculatorOptions.hpp"
#include "magnitudeService.hpp"

using namespace ULocalMagnitudeService::GRPC;

class Server::ServerImpl
{
public:
    ServerImpl(std::shared_ptr<spdlog::logger> logger) :
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
        createServices();
    }

    /// Run during construction
    void createServices()
    {   
std::unique_ptr<MagnitudeService> magnitudeService;
        mServicesMap.insert(
           std::move(
              std::pair {"MagnitudeService", std::move(magnitudeService)}
           )
        );
    }   

    void start()
    {
        grpc::ServerBuilder builder;
//        const auto address = mGRPCOptions.getHost() + ":" 
                           //+ std::to_string(mGRPCOptions.getPort());
        // Add global rules like messages size limits etc.
/*
        builder.SetMaxSendMessageSize(
            mOptions.getMaximumRequestMessageSizeInBytes());
        builder.SetOption(grpc::MakeChannelArgumentOption(
               "GRPC_ARG_MAX_CONNECTION_AGE_MS",
               static_cast<int>
                  (mOptions.getMaximumConnectionAge().count())));
        builder.SetOption(grpc::MakeChannelArgumentOption(
               "GRPC_ARG_MAX_CONNECTION_AGE_GRACE_MS",
               static_cast<int>
                   (mOptions.getMaximumConnectionAgeGracePeriod().count())));
        builder.SetOption(grpc::MakeChannelArgumentOption(
               "GRPC_ARG_MAX_CONCURRENT_STREAMS",
                mOptions.getMaximumNumberOfConcurrentStreams()));
*/
        // Secure the server?
if (true) 
//        if (mGRPCOptions.getServerKey() == std::nullopt ||
//            mGRPCOptions.getServerCertificate() == std::nullopt)
        {    
            SPDLOG_LOGGER_INFO(mLogger,
                "Initiating non-secured local magnitude service");
/*
            builder.AddListeningPort(address,
                                     grpc::InsecureServerCredentials());
            mSecured = false;
*/
        }
        else
        {
/*
            auto serverKey = mGRPCOptions.getServerKey();
            auto serverCertificate = mGRPCOptions.getServerCertificate();
#ifndef NDEBUG
            assert(serverKey != std::nullopt);
            assert(serverCertificate != std::nullopt);
#endif
*/
            SPDLOG_LOGGER_INFO(mLogger,
                "Initiating secured local magnitude service");
/*
            const grpc::SslServerCredentialsOptions::PemKeyCertPair keyCertPair
            {
                *serverKey,
                *serverCertificate
            };
            grpc::SslServerCredentialsOptions sslOptions;
            sslOptions.pem_key_cert_pairs.emplace_back(keyCertPair);
            builder.AddListeningPort(address,
                                     grpc::SslServerCredentials(sslOptions));
            mSecured = true;
*/
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
Server::Server(std::shared_ptr<spdlog::logger> logger) :
    pImpl(std::make_unique<ServerImpl> (std::move(logger)))
{
}

/// Destructor
Server::~Server() = default;
