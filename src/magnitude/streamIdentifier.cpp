#include <algorithm>
#include <cctype>
#include <locale>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"
//#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/stream_identifier.pb.h"

using namespace ULocalMagnitudeService::Magnitude;

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

class StreamIdentifier::StreamIdentifierImpl
{
public:
    std::string mNetwork;
    std::string mStation;
    std::string mChannel;
    std::string mLocationCode;
    bool mHasLocationCode{false};
};

/// Constructor
StreamIdentifier::StreamIdentifier() :
    pImpl(std::make_unique<StreamIdentifierImpl> ())
{
}

/// Copy constructor
StreamIdentifier::StreamIdentifier(const StreamIdentifier &identifier)
{
    *this = identifier;
}

/// Move constructor
StreamIdentifier::StreamIdentifier(StreamIdentifier &&identifier) noexcept
{
    *this = std::move(identifier);
}

/// Build from a protobuf
template<>
StreamIdentifier::StreamIdentifier(
    const ULocalMagnitudeServiceAPI::V1::Magnitude::StreamIdentifier
        &identifier)
{
    if (!identifier.has_network())
    {
        throw std::invalid_argument("Network not set on protobuf");
    }
    if (!identifier.has_station())
    {
        throw std::invalid_argument("Station not set on protobuf");
    }
    if (!identifier.has_channel())
    {
        throw std::invalid_argument("Channel not set on protobuf");
    }
    StreamIdentifier thisIdentifier;
    thisIdentifier.setNetwork(identifier.network());
    thisIdentifier.setStation(identifier.station());
    thisIdentifier.setChannel(identifier.channel());
    if (identifier.has_location_code())
    {
        thisIdentifier.setLocationCode(identifier.location_code());
    }
    else
    {
        thisIdentifier.setLocationCode("");
    }
    *this = std::move(thisIdentifier);
}

/// Copy assignment
StreamIdentifier 
&StreamIdentifier::operator=(const StreamIdentifier &identifier)
{
    if (&identifier == this){return *this;}
    pImpl = std::make_unique<StreamIdentifierImpl> (*identifier.pImpl);
    return *this;
}

/// Move assignment
StreamIdentifier 
&StreamIdentifier::operator=(StreamIdentifier &&identifier) noexcept
{
    if (&identifier == this){return *this;}
    pImpl = std::move(identifier.pImpl);
    return *this;
}

/// Destructor
StreamIdentifier::~StreamIdentifier() = default;

/// Network
void StreamIdentifier::setNetwork(const std::string &networkIn)
{
    auto network = ::removeBlanksAndCapitalize(networkIn);
    if (network.empty())
    {
        throw std::invalid_argument("Network is empty");
    }
    pImpl->mNetwork = network; 
}

std::string StreamIdentifier::getNetwork() const
{
    if (!hasNetwork()){throw std::runtime_error("Network not set");}
    return pImpl->mNetwork;
}

bool StreamIdentifier::hasNetwork() const noexcept
{
    return !pImpl->mNetwork.empty();
}

/// Station
void StreamIdentifier::setStation(const std::string &stationIn)
{
    auto station = ::removeBlanksAndCapitalize(stationIn);
    if (station.empty())
    {
        throw std::invalid_argument("Station is empty");
    }
    pImpl->mStation = station;
}

std::string StreamIdentifier::getStation() const
{
    if (!hasStation()){throw std::runtime_error("Station not set");}
    return pImpl->mStation;
}

bool StreamIdentifier::hasStation() const noexcept
{
    return !pImpl->mStation.empty();
}

/// Channel
void StreamIdentifier::setChannel(const std::string &channelIn)
{
    auto channel = ::removeBlanksAndCapitalize(channelIn);
    if (channel.empty())
    {
        throw std::invalid_argument("Channel is empty");
    }
    pImpl->mChannel = channel;
}   

std::string StreamIdentifier::getChannel() const
{
    if (!hasChannel()){throw std::runtime_error("Channel not set");}
    return pImpl->mChannel;
}
    
bool StreamIdentifier::hasChannel() const noexcept
{
    return !pImpl->mChannel.empty();
}

/// Location code
void StreamIdentifier::setLocationCode(const std::string &locationCodeIn)
{
    auto locationCode = ::removeBlanksAndCapitalize(locationCodeIn);
    pImpl->mLocationCode = locationCode;
    pImpl->mHasLocationCode = true;
}

std::string StreamIdentifier::getLocationCode() const
{
    if (!hasLocationCode()){throw std::runtime_error("Location code not set");}
    return pImpl->mLocationCode;
}

bool StreamIdentifier::hasLocationCode() const noexcept
{
    return pImpl->mHasLocationCode;
}


/// Name
std::string StreamIdentifier::toString() const
{
    auto result = getNetwork();
    result.append(".");
    result.append(getStation());
    result.append(".");
    result.append(getChannel());
    auto locationCode = getLocationCode();
    if (!locationCode.empty())
    {
        result.append(".");
        result.append(locationCode);
    }
    return result; 
}
 
/// Protobuf
template<>
ULocalMagnitudeServiceAPI::V1::Magnitude::StreamIdentifier
StreamIdentifier::toMessage() const
{
    ULocalMagnitudeServiceAPI::V1::Magnitude::StreamIdentifier result;
    result.set_network(getNetwork());
    result.set_station(getStation());
    result.set_channel(getChannel());
    result.set_location_code(getLocationCode());
    return result;
}


