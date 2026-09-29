#ifndef PROGRAM_OPTIONS_SERVICE_APPLICATION_HPP
#define PROGRAM_OPTIONS_SERVICE_APPLICATION_HPP
#include <chrono>
#include <string>
#include "uLocalMagnitudeService/corrections/distanceOptions.hpp"
#include "uLocalMagnitudeService/corrections/stationOptions.hpp"
#include "uLocalMagnitudeService/grpc/serverOptions.hpp"
#include "otelOptions.hpp"
#include "secretFile.hpp"

#define APPLICATION_NAME "uLocalMagnitudeService"

namespace
{

struct ProgramOptions
{
    ULocalMagnitudeService::OTelOptions::HTTPMetrics otelHTTPMetricsOptions;
    ULocalMagnitudeService::OTelOptions::HTTPLog otelHTTPLogOptions;
    ULocalMagnitudeService::OTelOptions::GRPCMetrics otelGRPCMetricsOptions;
    ULocalMagnitudeService::OTelOptions::GRPCLog otelGRPCLogOptions;
    std::string applicationName{APPLICATION_NAME};
    std::chrono::seconds printSummaryInterval{std::chrono::minutes {15}};
    ULocalMagnitudeService::GRPC::ServerOptions grpcServerOptions;
    int verbosity{3};
    bool exportLogs{false};
    bool exportLogsWithHTTP{true};
    bool exportMetrics{false};
    bool exportMetricsWithHTTP{true};
};


::ProgramOptions parseInitializationFile(const std::filesystem::path &iniFile)
{
    ::ProgramOptions options;
    if (!std::filesystem::exists(iniFile)){return options;}
    // Parse the initialization file
    boost::property_tree::ptree propertyTree;
    boost::property_tree::ini_parser::read_ini(iniFile, propertyTree);

    // Application name
    options.applicationName
        = propertyTree.get<std::string> ("General.applicationName",
                                         options.applicationName);
    if (options.applicationName.empty())
    {
        options.applicationName = APPLICATION_NAME;
    }   
    options.verbosity
        = propertyTree.get<int> ("General.verbosity", options.verbosity);

    auto summaryIntervalInMinutes
        = static_cast<int> (options.printSummaryInterval.count());
    summaryIntervalInMinutes
        = propertyTree.get<int> ("General.printSummaryIntervalInMinutes",
                                 summaryIntervalInMinutes);
    options.printSummaryInterval
        = std::chrono::minutes {summaryIntervalInMinutes};

    // GRPC data packet client options
    options.grpcServerOptions
        = ULocalMagnitudeService::GRPC::fromInitializationFile(iniFile);


    return options;
}


}
#endif
