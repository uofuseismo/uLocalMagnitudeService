#include <atomic>
#ifndef NDEBUG
#include <cassert>
#endif
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <stdlib.h>
#include <thread>
#include <utility>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h> //NOLINT
#include "uLocalMagnitudeService/metrics/singleton.hpp"
#include "uLocalMagnitudeService/version.hpp"
#include "logger.hpp"
#include "metrics.hpp"
#include "parseCommandLineOptions.hpp"
#include "programOptions.hpp"

namespace
{

volatile std::sig_atomic_t mSignalStatus;
std::atomic_bool mInterrupted{false};

class Process
{
public:
    Process(::ProgramOptions &&options,
            std::shared_ptr<spdlog::logger> logger) :
        mOptions(std::move(options)),
        mLogger(std::move(logger))
    {
#ifndef NDEBUG
        assert(mLogger != nullptr);
#endif      

    }

    /// @brief Destructor
    ~Process()
    {
        stop();
    }

    /// @brief Starts processes
    void start()
    {
        mKeepRunning.store(true);
        handleMainThread();
    }

    /// @brief Stops processes
    void stop()
    {
        mKeepRunning.store(false);
    }

    /// @brief Peridically prints the status to a summary log
    void printSummary()
    {
        /// Summary 
        if (mOptions.printSummaryInterval.count() <= 0){return;}

    }

    /// @brief Checks any futures
    [[nodiscard]] bool areFuturesOkay(const std::chrono::milliseconds &) const
    {
        return true;
    }

    /// @brief Handles main thread activities.
    void handleMainThread()
    {
        SPDLOG_LOGGER_INFO(mLogger, "Main thread entering waiting loop");
        catchSignals();
        while (!mStopProcessRequested)
        {
            if (mInterrupted)
            {
                SPDLOG_LOGGER_INFO(mLogger,
                                  "SIGINT/SIGTERM signal received!");
                mStopProcessRequested = true;
                break;
            }
            constexpr std::chrono::milliseconds waitForFuture {5};
            if (!areFuturesOkay(waitForFuture))
            {
                SPDLOG_LOGGER_CRITICAL(mLogger,
                   "Futures exception caught; terminating app");
                mStopProcessRequested = true;
                break;
            }
            printSummary();
            std::unique_lock<std::mutex> lock(mStopMutex);
            constexpr std::chrono::milliseconds pause{100};
            mStopProcessCondition.wait_for(lock, pause,
                                           [this]
                                           {
                                               return mStopProcessRequested;
                                           });
        }
        if (mStopProcessRequested)
        {
            SPDLOG_LOGGER_DEBUG(mLogger,
                                "Stop request received.  Terminating...");
            stop();
            std::this_thread::sleep_for(std::chrono::milliseconds {15});
        }
    }

    /// @brief Defines the signals we'll react to.
    void catchSignals()
    {
        std::signal(SIGINT,  Process::signalHandler);
        std::signal(SIGTERM, Process::signalHandler);
    }

    static void signalHandler(const int signal)
    {
        mSignalStatus = signal;
        mInterrupted.store(true);
    }

//private:
    ProgramOptions mOptions; 
    std::shared_ptr<spdlog::logger> mLogger{nullptr};
    mutable std::mutex mStopMutex;
    std::condition_variable mStopProcessCondition;
    std::condition_variable mTerminateCondition;
    bool mStopProcessRequested{false};
    bool mTerminateRequested{false};
    std::atomic<bool> mKeepRunning{false};
};

}

int main(int argc, char *argv[])
{
    // Intialization and welcome
    namespace ULM = ULocalMagnitudeService;
    ULM::Metrics::initializeSingleton(); 
    //NOLINTNEXTLINE(misc-include-cleaner)
    auto consoleLogger = spdlog::stdout_color_st("console");
    SPDLOG_LOGGER_INFO(consoleLogger,
                       "Running version {} of uLocalMagnitudeService",
                       ULM::Version::getVersionWithTag());

    // Parse the command line arguments
    std::filesystem::path iniFile;
    try 
    {   
        auto [iniFileName, isHelp] = ::parseCommandLineOptions(argc, argv);
        if (isHelp){return EXIT_SUCCESS;}
        if (iniFileName.empty())
        {
            throw std::runtime_error("No initialization file specified");
        }
        iniFile = iniFileName;
    }   
    catch (const std::exception &e) 
    {   
        SPDLOG_LOGGER_CRITICAL(consoleLogger,
                               "Failed to read command line options because {}",
                               std::string {e.what()});
        return EXIT_FAILURE;
    }

    // Get the program options
    ::ProgramOptions programOptions;
    try
    {
        programOptions = ::parseInitializationFile(iniFile);
    }
    catch (const std::exception &e)
    {
        SPDLOG_LOGGER_CRITICAL(consoleLogger,
                               "Failed to read program options because {}",
                               std::string {e.what()});
        return EXIT_FAILURE;
    }

    if ((programOptions.exportLogs || programOptions.exportMetrics) &&
        std::getenv("OTEL_SERVICE_NAME") == nullptr)
    {
        constexpr int overwrite{1};
        setenv("OTEL_SERVICE_NAME",
               programOptions.applicationName.c_str(),
               overwrite);
    }

    // Create the real logger
    std::shared_ptr<spdlog::logger> logger{nullptr};
    try
    {
        logger = ULM::Logger::initialize(programOptions);
    }
    catch (const std::exception &e)
    {
        //NOLINTNEXTLINE(misc-include-cleaner)
        auto consoleLogger = spdlog::stdout_color_st("console");
        SPDLOG_LOGGER_CRITICAL(consoleLogger,
                               "Failed to initialize logger because {}",
                               std::string {e.what()});
        return EXIT_FAILURE;
    }

    try
    {
        ULM::Metrics::initialize(programOptions);
    }
    catch (const std::exception &e)
    {
        SPDLOG_LOGGER_CRITICAL(logger,
                               "Failed to initialize metrics because {}",
                               std::string {e.what()});
        ULM::Logger::cleanup();
        return EXIT_FAILURE;
    }

    std::unique_ptr<::Process> process{nullptr};
    try 
    {
        SPDLOG_LOGGER_INFO(logger, "Initializing main process");
        process
            = std::make_unique<::Process> (std::move(programOptions), logger);
    }
    catch (const std::exception &e) 
    {   
        SPDLOG_LOGGER_CRITICAL(logger,
                               "Failed to initialize main process because {}",
                               std::string {e.what()});
        ULM::Metrics::cleanup();
        ULM::Logger::cleanup();
        return EXIT_FAILURE;
    }

    try 
    {   
        process->start();
        ULM::Metrics::cleanup();
        ULM::Logger::cleanup();
    }   
    catch (const std::exception &e) 
    {   
        SPDLOG_LOGGER_CRITICAL(logger,
                               "Application failed because {}",
                               std::string {e.what()});
        ULM::Metrics::cleanup();
        ULM::Logger::cleanup();
        return EXIT_FAILURE;
    }   
    return EXIT_SUCCESS;


    return EXIT_SUCCESS;
}
