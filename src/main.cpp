#include <cstdlib>
#include <exception>
#include <filesystem>
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h> //NOLINT
#include "uLocalMagnitudeService/metrics/singleton.hpp"
#include "uLocalMagnitudeService/version.hpp"
#include "logger.hpp"
#include "parseCommandLineOptions.hpp"

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

    return EXIT_SUCCESS;
}
