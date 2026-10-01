#include <exception>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "uLocalMagnitudeService/magnitude/networkMagnitudeCalculator.hpp"
#include "uLocalMagnitudeService/magnitude/amplitude.hpp"
#include "uLocalMagnitudeService/magnitude/networkMagnitudeCalculatorOptions.hpp"
#include "uLocalMagnitudeService/magnitude/observation.hpp"
#include "uLocalMagnitudeService/magnitude/stationMagnitudeCalculator.hpp"
#include "uLocalMagnitudeService/corrections/distance.hpp"
// NOLINTBEGIN(misc-include-cleaner)
#include "uLocalMagnitudeService/corrections/station.hpp"
// NOLINTEND(misc-include-cleaner)
#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"
#include "uLocalMagnitudeService/corrections/stationsSet.hpp"

using namespace ULocalMagnitudeService::Magnitude;



class NetworkMagnitudeCalculator::NetworkMagnitudeCalculatorImpl
{
public:
    NetworkMagnitudeCalculatorOptions mOptions;
    std::map<std::string, StationMagnitudeCalculator> mStationMagnitudeCalculatorMap;
    bool mInitialized{false};
};


/// Constructor
NetworkMagnitudeCalculator::NetworkMagnitudeCalculator(
    const NetworkMagnitudeCalculatorOptions &options) :
    pImpl(std::make_unique<NetworkMagnitudeCalculatorImpl> ())
{
    try
    {
        options.validate();
    }
    catch (const std::exception &e)
    {
        throw std::invalid_argument(
            "Network magnitude options invalid because "
          + std::string {e.what()});
    }
    const auto distanceCorrections = options.getDistanceCorrections();
    const auto stationCorrections
        = options.getStationCorrections().getCorrections();
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
        pImpl->mStationMagnitudeCalculatorMap.insert(std::move(calculatorPair));
    }
    pImpl->mOptions = options; 
    pImpl->mInitialized = true;
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
        return std::make_optional<double> (it->second.getStationCorrection());
    }
    return std::nullopt;
}

/// Destructor
NetworkMagnitudeCalculator::~NetworkMagnitudeCalculator() = default;
