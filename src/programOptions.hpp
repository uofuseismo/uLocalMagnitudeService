#ifndef PROGRAM_OPTIONS_SERVICE_APPLICATION_HPP
#define PROGRAM_OPTIONS_SERVICE_APPLICATION_HPP
#include <chrono>
#include <string>
#include <boost/algorithm/string/case_conv.hpp>
#include "uLocalMagnitudeService/corrections/distance.hpp"
#include "uLocalMagnitudeService/corrections/distanceOptions.hpp"
#include "uLocalMagnitudeService/corrections/stationsSet.hpp"
#include "uLocalMagnitudeService/magnitude/networkMagnitudeCalculatorOptions.hpp"
//#include "uLocalMagnitudeService/magnitude/stationMagnitudeCalculatorOptions.hpp"
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
    ULocalMagnitudeService::Magnitude::NetworkMagnitudeCalculatorOptions
        networkMagnitudeCalculatorOptions;
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

    // Network magnitude options
    ULocalMagnitudeService::Magnitude::NetworkMagnitudeCalculatorOptions
        networkMagnitudeCalculatorOptions;
    auto minStationMagnitudes
        = propertyTree.get<int>
          ("NetworkMagnitude.minimumNumberOfStationMagnitudes",
           networkMagnitudeCalculatorOptions.getMinimumNumberOfStationMagnitudes());
    networkMagnitudeCalculatorOptions.setMinimumNumberOfStationMagnitudes(
        minStationMagnitudes);
    auto networkMagnitudeStrategy
        = propertyTree.get<std::string>
          ("NetworkMagnitude.strategy", "average");
    boost::algorithm::to_lower(networkMagnitudeStrategy);
    if (networkMagnitudeStrategy == "average")
    {
        networkMagnitudeCalculatorOptions.setStrategy(
           ULocalMagnitudeService::Magnitude::NetworkMagnitudeCalculatorOptions
              ::Strategy::Average);
    }
    else
    {
        throw std::invalid_argument("Unhandled network mag strategy " 
                                  + networkMagnitudeStrategy 
                                  + "; only 'average' implemented");
    }
    auto distanceCorrectionsOptions
        = ULocalMagnitudeService::Corrections::DistanceOptions::
            fromInitializationFile(iniFile, "DistanceCorrections");
    ULocalMagnitudeService::Corrections::Distance
        distanceCorrections{distanceCorrectionsOptions};
    networkMagnitudeCalculatorOptions.setDistanceCorrections(distanceCorrections);
    auto stationCorrections
        = ULocalMagnitudeService::Corrections::StationsSet::
            fromInitializationFile(iniFile, "StationCorrections");
    networkMagnitudeCalculatorOptions.setStationCorrections(stationCorrections);

    // GRPC data packet client options
    options.grpcServerOptions
        = ULocalMagnitudeService::GRPC::fromInitializationFile(iniFile);

    // Get OTel logs options
    auto httpLog
        = ULocalMagnitudeService::OTelOptions::getHTTPLogOptionsFromIniFile(
                propertyTree, "OTelHTTPLogOptions");
    options.exportLogs = false;
    if (httpLog != std::nullopt)
    {
        options.otelHTTPLogOptions = *httpLog;
        options.exportLogs = true;
        options.exportLogsWithHTTP = true;
    }
    else
    {
        auto grpcLog
            = ULocalMagnitudeService::OTelOptions::getGRPCLogOptionsFromIniFile(
                propertyTree, "OTelGRPCLogOptions");
        if (grpcLog != std::nullopt)
        {
            options.otelGRPCLogOptions = *grpcLog;
            options.exportLogs = true;
            options.exportLogsWithHTTP = false;
        }
    }

    // Get OTel metrics options
    auto httpMetrics
        = ULocalMagnitudeService::OTelOptions::getHTTPMetricsOptionsFromIniFile(
                propertyTree, "OTelHTTPMetricsOptions");
    options.exportMetrics = false;
    if (httpMetrics != std::nullopt)
    {
        options.otelHTTPMetricsOptions = *httpMetrics;
        options.exportMetrics = true;
        options.exportMetricsWithHTTP = true;
    }
    else
    {
        auto grpcMetrics
            = ULocalMagnitudeService::OTelOptions::
                getGRPCMetricsOptionsFromIniFile(
                    propertyTree, "OTelGRPCMetricsOptions");
        if (grpcMetrics != std::nullopt)
        {
            options.otelGRPCMetricsOptions = *grpcMetrics;
            options.exportMetrics = true;
            options.exportMetricsWithHTTP = false;
        }
    }

    return options;
}


}
#endif
