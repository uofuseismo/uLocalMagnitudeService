#include <exception>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "uLocalMagnitudeService/magnitude/network.hpp"
#include "uLocalMagnitudeService/magnitude/amplitude.hpp"
#include "uLocalMagnitudeService/magnitude/networkOptions.hpp"
#include "uLocalMagnitudeService/magnitude/observation.hpp"
#include "uLocalMagnitudeService/magnitude/station.hpp"
#include "uLocalMagnitudeService/corrections/distance.hpp"
// NOLINTBEGIN(misc-include-cleaner)
#include "uLocalMagnitudeService/corrections/station.hpp"
// NOLINTEND(misc-include-cleaner)
#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"
#include "uLocalMagnitudeService/corrections/stationsSet.hpp"

using namespace ULocalMagnitudeService::Magnitude;



class Network::NetworkImpl
{
public:
    NetworkOptions mOptions;
    std::map<std::string, Magnitude::Station> mStationMagnitudeCalculatorMap;
    bool mInitialized{false};
};


/// Constructor
Network::Network(const NetworkOptions &options) :
    pImpl(std::make_unique<NetworkImpl> ())
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
        Magnitude::Station
            stationMagnitude{stationCorrection, distanceCorrections};
        if (!stationMagnitude.isInitialized())
        {
            throw std::runtime_error(
                "Failed to initialize station magnitude calculator for "
              + stationName);
        }
        std::pair<std::string, Magnitude::Station> calculatorPair
        {
            stationName,
            std::move(stationMagnitude)
        };
        pImpl->mStationMagnitudeCalculatorMap.insert(std::move(calculatorPair));
    }
    pImpl->mOptions = options; 
    pImpl->mInitialized = true;
}

/// Move constructor
Network::Network(Network &&network) noexcept
{
    *this = std::move(network);
}

/// Move assignment
Network& Network::operator=(Network &&network) noexcept
{ 
    if (&network == this){return *this;}
    pImpl = std::move(network.pImpl);
    return *this;
}

/// Initialized?
bool Network::isInitialized() const noexcept
{
    return pImpl->mInitialized;
}

/// The distance corerections
ULocalMagnitudeService::Corrections::Distance 
    Network::getDistanceCorrections() const
{
    if (!isInitialized())
    {
        throw std::runtime_error("Network magnitude not initialized");
    }
    return pImpl->mOptions.getDistanceCorrections();
}

/// The station correction
std::optional<double> 
Network::getStationCorrection(
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
Network::~Network() = default;
