#include <expected>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include "uLocalMagnitude/corrections/stations.hpp"
#include "uLocalMagnitude/corrections/station.hpp"
#include "uLocalMagnitude/corrections/stationIdentifier.hpp"
#include "uLocalMagnitude/corrections/stationsSet.hpp"

using namespace ULocalMagnitude::Corrections;

class Stations::StationsImpl
{
public:
    StationsSet mCorrections;
    bool mInitialized{false};
};

/// Constructor
Stations::Stations(const StationsSet &set) :
    pImpl(std::make_unique<StationsImpl> ())
{
    if (set.empty())
    {
        throw std::invalid_argument("No station corrections");
    }
    pImpl->mCorrections = set;
    pImpl->mInitialized = true;
}

/// Copy constructor
Stations::Stations(const Stations &stations)
{
    *this = stations;
}

/// Move constructor
Stations::Stations(Stations &&stations) noexcept
{
    *this = std::move(stations);
}

/// Copy assignent
Stations& Stations::operator=(const Stations &stations)
{
    if (&stations == this){return *this;}
    pImpl = std::make_unique<StationsImpl> (*stations.pImpl);
    return *this;
}

/// Move assignent
Stations& Stations::operator=(Stations &&stations) noexcept
{
    if (&stations == this){return *this;}
    pImpl = std::move(stations.pImpl);
    return *this;
}

/// Destructor
Stations::~Stations() = default;

/// Do it
std::expected<double, Stations::ErrorCode>
Stations::operator()(const StationIdentifier &identifier) const
{
    if (!isInitialized())
    {   
        return std::unexpected(Stations::ErrorCode::Uninitialized);
    }   
    try
    {
        return this->operator()(identifier.toString());
    }
    catch (...)
    {
        return std::unexpected(Stations::ErrorCode::InvalidStation);
    }
}

/// Initialized
bool Stations::isInitialized() const noexcept
{
    return pImpl->mInitialized;
}

std::expected<double, Stations::ErrorCode> 
Stations::operator()(const std::string &identifier) const
{
    if (!isInitialized())
    {   
        return std::unexpected(Stations::ErrorCode::Uninitialized);
    }   
    if (identifier.empty())
    {
        return std::unexpected(Stations::ErrorCode::InvalidStation);
    }
    auto correction = pImpl->mCorrections.getCorrection(identifier);
    if (correction != std::nullopt)
    {
        return correction->operator()();
    }
    return std::unexpected(Stations::ErrorCode::StationDoesNotExist);
}

/// Get corrections - for debugging
std::map<std::string, double> Stations::getCorrections() const
{
    if (!isInitialized())
    {   
        throw std::runtime_error("Stations corrections not initialized");
    }   
    std::map<std::string, double> result;
    auto corrections = pImpl->mCorrections.getCorrections();
    for (const auto &correction : corrections)
    {
        result.insert_or_assign(
           correction.second.getName(),
           correction.second.operator()());
    }
    return result;
} 
