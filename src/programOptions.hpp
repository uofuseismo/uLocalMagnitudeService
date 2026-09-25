#ifndef PROGRAM_OPTIONS_SERVICE_APPLICATION_HPP
#define PROGRAM_OPTIONS_SERVICE_APPLICATION_HPP
#include <chrono>
#include "otelOptions.hpp"

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
    int verbosity{3};
    bool exportLogs{false};
    bool exportLogsWithHTTP{true};
    bool exportMetrics{false};
    bool exportMetricsWithHTTP{true};
};
}
#endif
