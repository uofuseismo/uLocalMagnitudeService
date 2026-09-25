#include <algorithm>
#include <cctype>
#include <locale>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include "uLocalMagnitude/corrections/stationIdentifier.hpp"

using namespace ULocalMagnitude::Corrections;

namespace
{
std::string removeBlanksAndCapitalize(const std::string &input)
{
    const auto &locale = std::locale::classic();
    auto result = input;
    result.erase(
        std::remove_if(result.begin(), result.end(),
                       [&locale](const char c)
                       {
                           return std::isspace(c, locale);
                       }),
        result.end());
    std::transform(result.begin(), result.end(), result.begin(),
                   [&locale](const char c)
                   {
                       return std::toupper(c, locale);
                   });
    return result;
}
}

class StationIdentifier::StationIdentifierImpl
{
public:
    std::string mNetwork;
    std::string mStation;
};

/// Constructor
StationIdentifier::StationIdentifier() :
    pImpl(std::make_unique<StationIdentifierImpl> ())
{
}

/// Copy constructor
StationIdentifier::StationIdentifier(const StationIdentifier &identifier)
{
    *this = identifier;
}

/// Move constructor
StationIdentifier::StationIdentifier(StationIdentifier &&identifier) noexcept
{
    *this = std::move(identifier);
}

/// Copy assignment
StationIdentifier 
&StationIdentifier::operator=(const StationIdentifier &identifier)
{
    if (&identifier == this){return *this;}
    pImpl = std::make_unique<StationIdentifierImpl> (*identifier.pImpl);
    return *this;
}

/// Move assignment
StationIdentifier 
&StationIdentifier::operator=(StationIdentifier &&identifier) noexcept
{
    if (&identifier == this){return *this;}
    pImpl = std::move(identifier.pImpl);
    return *this;
}

/// Destructor
StationIdentifier::~StationIdentifier() = default;

/// Network
void StationIdentifier::setNetwork(const std::string &networkIn)
{
    auto network = ::removeBlanksAndCapitalize(networkIn);
    if (network.empty())
    {
        throw std::invalid_argument("Network is empty");
    }
    pImpl->mNetwork = network; 
}

std::string StationIdentifier::getNetwork() const
{
    if (!hasNetwork()){throw std::runtime_error("Network not set");}
    return pImpl->mNetwork;
}

bool StationIdentifier::hasNetwork() const noexcept
{
    return !pImpl->mNetwork.empty();
}

/// Station
void StationIdentifier::setStation(const std::string &stationIn)
{
    auto station= ::removeBlanksAndCapitalize(stationIn);
    if (station.empty())
    {
        throw std::invalid_argument("Station is empty");
    }
    pImpl->mStation = station;
}

std::string StationIdentifier::getStation() const
{
    if (!hasStation()){throw std::runtime_error("Station not set");}
    return pImpl->mStation;
}

bool StationIdentifier::hasStation() const noexcept
{
    return !pImpl->mStation.empty();
}

/// Name
std::string StationIdentifier::toString() const
{
    auto result = getNetwork();
    result.append(".");
    result.append(getStation());
    return result; 
}
 
