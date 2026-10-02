#include <algorithm>
#include <exception>
#include <expected>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
//NOLINTNEXTLINE(misc-include-cleaner)
#include <spdlog/sinks/stdout_color_sinks.h>
#include "uLocalMagnitudeService/magnitude/networkMagnitudeCalculator.hpp"
#include "uLocalMagnitudeService/magnitude/amplitude.hpp"
#include "uLocalMagnitudeService/magnitude/networkMagnitude.hpp"
#include "uLocalMagnitudeService/magnitude/networkMagnitudeCalculatorOptions.hpp"
#include "uLocalMagnitudeService/magnitude/stationMagnitude.hpp"
#include "uLocalMagnitudeService/magnitude/observation.hpp"
#include "uLocalMagnitudeService/magnitude/stationMagnitudeCalculator.hpp"
#include "uLocalMagnitudeService/corrections/distance.hpp"
#include "uLocalMagnitudeService/corrections/distanceOptions.hpp"
// NOLINTBEGIN(misc-include-cleaner)
#include "uLocalMagnitudeService/corrections/station.hpp"
// NOLINTEND(misc-include-cleaner)
#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"
#include "uLocalMagnitudeService/corrections/stationsSet.hpp"

using namespace ULocalMagnitudeService::Magnitude;



class NetworkMagnitudeCalculator::NetworkMagnitudeCalculatorImpl
{
public:
    NetworkMagnitudeCalculatorImpl() = default;
    NetworkMagnitudeCalculatorImpl(const NetworkMagnitudeCalculatorImpl &) = default;

    NetworkMagnitudeCalculatorImpl
    (
        const NetworkMagnitudeCalculatorOptions &options,
        std::shared_ptr<spdlog::logger> logger
    ) :
        mOptions(options),
        mLogger(std::move(logger))
    {
        try 
        {   
            mOptions.validate();
        }   
        catch (const std::exception &e) 
        {   
            throw std::invalid_argument(
                "Network magnitude options invalid because "
              + std::string {e.what()});
        }   
        if (mLogger == nullptr)
        {   
            // NOLINTBEGIN(misc-include-cleaner)
            constexpr const char *loggerName
            {
                "NetworkMagnitudeCalculatorConsole"
            };
            mLogger = spdlog::get(loggerName);
            if (mLogger == nullptr)
            {   
                mLogger = spdlog::stdout_color_mt(loggerName);
            }   
            // NOLINTEND(misc-include-cleaner)
        }   

        const auto distanceCorrections = mOptions.getDistanceCorrections();
        const auto stationCorrections
            = mOptions.getStationCorrections().getCorrections();
        for (const auto &stationCorrectionPair : stationCorrections)
        {
            const auto stationName = stationCorrectionPair.first;
            const auto &stationCorrection = stationCorrectionPair.second;
            StationMagnitudeCalculator
                calculator{stationCorrection, distanceCorrections};
            if (!calculator.isInitialized())
            {
                throw std::runtime_error(
                    "Failed to initialize station magnitude calculator for "
                  + stationName);
            }
            std::pair<std::string, StationMagnitudeCalculator> calculatorPair
            {
                stationName,
                std::move(calculator)
            };
            mStationMagnitudeCalculatorMap.insert(std::move(calculatorPair));
        }
        mRequiresDepthCorrection = false;
        if (distanceCorrections.getDistanceType() ==
            Corrections::DistanceOptions::Type::Hypocentral)
        {
            mRequiresDepthCorrection = true;
        }
        mInitialized = true;
    }
    NetworkMagnitudeCalculatorOptions mOptions;
    std::shared_ptr<spdlog::logger> mLogger{nullptr};
    std::map<std::string, StationMagnitudeCalculator> mStationMagnitudeCalculatorMap;
    bool mRequiresDepthCorrection{false};
    bool mInitialized{false};
};


/// Constructor
NetworkMagnitudeCalculator::NetworkMagnitudeCalculator(
    const NetworkMagnitudeCalculatorOptions &options,
    std::shared_ptr<spdlog::logger> logger) :
    pImpl(std::make_unique<NetworkMagnitudeCalculatorImpl>
          (options, std::move(logger)))
{
}

/// Copy constructor
NetworkMagnitudeCalculator::NetworkMagnitudeCalculator(
    const NetworkMagnitudeCalculator &calculator)
{
    *this = calculator;
}

/// Move constructor
NetworkMagnitudeCalculator::NetworkMagnitudeCalculator(
    NetworkMagnitudeCalculator &&calculator) noexcept
{
    *this = std::move(calculator);
}

/// Copy assignment
NetworkMagnitudeCalculator&
NetworkMagnitudeCalculator::operator=(
    const NetworkMagnitudeCalculator &calculator)
{
    if (&calculator == this){return *this;}
    pImpl = std::make_unique<NetworkMagnitudeCalculatorImpl>
            (*calculator.pImpl);
    return *this;
}

/// Move assignment
NetworkMagnitudeCalculator&
NetworkMagnitudeCalculator::operator=(
    NetworkMagnitudeCalculator &&calculator) noexcept
{ 
    if (&calculator == this){return *this;}
    pImpl = std::move(calculator.pImpl);
    return *this;
}

/// Initialized?
bool NetworkMagnitudeCalculator::isInitialized() const noexcept
{
    return pImpl->mInitialized;
}

/// The distance corerections
ULocalMagnitudeService::Corrections::Distance 
    NetworkMagnitudeCalculator::getDistanceCorrections() const
{
    if (!isInitialized())
    {
        throw std::runtime_error("Network magnitude not initialized");
    }
    return pImpl->mOptions.getDistanceCorrections();
}

/// The station correction
std::optional<double> 
NetworkMagnitudeCalculator::getStationCorrection(
    const Corrections::StationIdentifier &identifier) const
{
    if (!isInitialized())
    {   
        throw std::runtime_error("Network magnitude not initialized");
    }   
    const auto stationName = identifier.toString();
    const auto it = pImpl->mStationMagnitudeCalculatorMap.find(stationName);
    if (it != pImpl->mStationMagnitudeCalculatorMap.end())
    {
        auto correction = it->second.getStationCorrection();
        if (correction.has_value())
        {
            return *correction;
        }
        else if (correction.error() ==
                 StationMagnitudeCalculator::ErrorCode::Algorithmic)
        {
            throw std::runtime_error("Alglorihmic error in station correction");
        }
        else if (correction.error() ==
                 StationMagnitudeCalculator::ErrorCode::Uninitialized)
        {
            throw std::runtime_error("Failed to initialize station correction");
        }
        else
        {
            throw std::runtime_error("Algorithmic error");
        } 
    }
    return std::nullopt;
}

/// Destructor
NetworkMagnitudeCalculator::~NetworkMagnitudeCalculator() = default;

/// Apply
NetworkMagnitude
NetworkMagnitudeCalculator::operator()(
    const std::vector<Observation> &observations) const
{
    if (observations.empty())
    {
        throw std::invalid_argument("No observations provided");
    }
    // Check the observations
    for (const auto &observation : observations)
    {
        if (!observation.hasAmplitudes())
        {
            throw std::invalid_argument("Observation missing amplitudes");
        }
        if (!observation.hasEpicentralDistance())
        {
            throw std::invalid_argument(
                observation.getStationName() + " missing epicentral distance");
        }
        if (pImpl->mRequiresDepthCorrection && !observation.hasDepth())
        {
            throw std::invalid_argument(
                observation.getStationName() + " missing source depth");
        }
    }
    // Deal with duplicates (can't repeat an observation at a site) and
    // stations missing a station correction
    struct WorkItem
    {   
        Observation observation;
        std::vector<int> observationIndices;
        std::expected<StationMagnitude, StationMagnitudeCalculator::ErrorCode> stationMagnitude;
    };  
    std::vector<WorkItem> workItems;
    workItems.reserve(observations.size());
    auto nObservations = static_cast<int> (observations.size());
    std::vector<int> matchedStation(nObservations, -1);
    std::vector<bool> hasStationCorrection(nObservations, false); 
    for (int i = 0; i < nObservations; ++i)
    {
        if (matchedStation[i] != -1){continue;} // Already got it
        auto stationName_i = observations[i].getStationName();
        hasStationCorrection[i]
            = pImpl->mStationMagnitudeCalculatorMap.contains(stationName_i);
        if (matchedStation[i] != -1){continue;} // Already got it
        std::vector<int> observationIndices;
        observationIndices.reserve(nObservations);
        observationIndices.push_back(i);
        for (int j = i + 1; j < nObservations; ++j)
        {
            if (stationName_i == observations[j].getStationName())
            {
                observationIndices.push_back(j);
                hasStationCorrection[j] = hasStationCorrection[i];
                matchedStation[j] = i;
            }
        }
        if (observationIndices.size() > 1)
        {
            SPDLOG_LOGGER_WARN(pImpl->mLogger,
                "{} observed {} times - using the first observation",
                stationName_i, observationIndices.size());
        }
        if (!hasStationCorrection[i])
        {
            SPDLOG_LOGGER_WARN(pImpl->mLogger,
                "No station correction for {} - skipping", stationName_i);
        }
        // Unique and by this point not done - let's do it
        if (hasStationCorrection[i] && matchedStation[i] == -1)
        {
            WorkItem workItem
            {
                observations[i],
                std::move(observationIndices)
            };
            workItems.push_back(std::move(workItem));
        }   
    }
    // We can quit now (can still get worse)
    auto minObservationsRequired
        = pImpl->mOptions.getMinimumNumberOfStationMagnitudes();
    if (static_cast<int> (workItems.size()) < minObservationsRequired)
    {
        throw std::invalid_argument("Require at least "
                                  + std::to_string(minObservationsRequired)
                                  + " unique station magnitudes"); 
    }
    // Compute the station magnitudes
    auto computeStationMagnitude = [&](WorkItem &workItem)
    {
        auto stationName = workItem.observation.getStationName();
        auto it = pImpl->mStationMagnitudeCalculatorMap.find(stationName); 
        if (it != pImpl->mStationMagnitudeCalculatorMap.end())
        { 
            workItem.stationMagnitude
                = it->second.operator()(workItem.observation);
        }
    };
    std::ranges::for_each(workItems.begin(), workItems.end(),
                          computeStationMagnitude);
    // Now to combine the magnitudes
    if (pImpl->mOptions.getStrategy() !=
        NetworkMagnitudeCalculatorOptions::Strategy::Average)
    {
        throw std::runtime_error("Unhandled network magnitude strategy");
    }
    // Average every station magnitude we could compute - no trimming
    int nStationMagnitudes{0};
    double sum{0};
    for (const auto &item : workItems)
    {
        if (item.stationMagnitude.has_value())
        {
            nStationMagnitudes = nStationMagnitudes + 1;
            sum = sum + item.stationMagnitude->getValue();
        }
        else
        {
            SPDLOG_LOGGER_WARN(pImpl->mLogger,
                "Failed to compute station magnitude for {} (error code {})",
                item.observation.getStationName(),
                static_cast<int> (item.stationMagnitude.error()));
        }
    }
    if (nStationMagnitudes < std::max(1, minObservationsRequired))
    {
        throw std::invalid_argument("Computed "
                                  + std::to_string(nStationMagnitudes)
                                  + " station magnitudes but require at least "
                                  + std::to_string(minObservationsRequired));
    }
    NetworkMagnitude networkMagnitude;
    networkMagnitude.setValue(sum/static_cast<double> (nStationMagnitudes));
    return networkMagnitude;
}
