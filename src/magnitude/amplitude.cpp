#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include "uLocalMagnitudeService/magnitude/amplitude.hpp"
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/amplitude.pb.h"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/stream_identifier.pb.h"

using namespace ULocalMagnitudeService::Magnitude;

class Amplitude::AmplitudeImpl
{
public:
    StreamIdentifier mIdentifier;
    double mValue{-1};
    bool mHasIdentifier{false};
};

/// Constructor
Amplitude::Amplitude() :
    pImpl(std::make_unique<AmplitudeImpl> ())
{
}

/// Copy constructor
Amplitude::Amplitude(const Amplitude &amplitude)
{
    *this = amplitude;
}

/// Move constructor
Amplitude::Amplitude(Amplitude &&amplitude) noexcept
{
    *this = std::move(amplitude);
}

/// Build from a protobuf
template<>
Amplitude::Amplitude(
    const ULocalMagnitudeServiceAPI::V1::Magnitude::Amplitude &amplitude)
{
    if (!amplitude.has_stream_identifier())
    {   
        throw std::invalid_argument("Stream identifier not set.");
    }   
    if (!amplitude.has_value())
    {   
        throw std::invalid_argument("Amplitude value not set.");
    }   
    if (!amplitude.has_units())
    {   
        throw std::invalid_argument("Amplitude units not set.");
    }   
    namespace API = ULocalMagnitudeServiceAPI::V1::Magnitude;
    Amplitude thisAmplitude;
    auto units = amplitude.units();
    if (units == API::Amplitude_Units_UNKNOWN)
    {
        throw std::invalid_argument("Units not specified");
    }
    else if (units == API::Amplitude_Units_MILLIMETERS)
    {
        thisAmplitude.setValue(amplitude.value());
    }
    else if (units == API::Amplitude_Units_CENTIMETERS)
    {
        thisAmplitude.setValue(amplitude.value()*10);
    }
    else if (units == API::Amplitude_Units_METERS)
    {
        thisAmplitude.setValue(amplitude.value()*1000);
    }
    else
    {
        throw std::runtime_error("Unhandled units logic");
    }
    StreamIdentifier streamIdentifier{amplitude.stream_identifier()};
    thisAmplitude.setIdentifier(std::move(streamIdentifier));
    *this = std::move(thisAmplitude);
}

/// Copy assignment
Amplitude &Amplitude::operator=(const Amplitude &amplitude)
{
    if (&amplitude == this){return *this;}
    pImpl = std::make_unique<AmplitudeImpl> (*amplitude.pImpl);
    return *this;
}

/// Move assignment
Amplitude &Amplitude::operator=(Amplitude &&amplitude) noexcept
{
    if (&amplitude == this){return *this;}
    pImpl = std::move(amplitude.pImpl);
    return *this;
}

/// Destructor
Amplitude::~Amplitude() = default;

/// Value
void Amplitude::setValue(const double value)
{
    if (!std::isfinite(value))
    {
        throw std::invalid_argument("Amplitude is not finite");
    }
    if (value <= 0)
    {
        throw std::invalid_argument("Amplitude value must be positive");
    }
    pImpl->mValue = value;
}

double Amplitude::getValue() const
{
    if (!hasValue())
    {
        throw std::runtime_error("Amplitude value not set");
    }
    return pImpl->mValue;
}

bool Amplitude::hasValue() const noexcept
{
    return (pImpl->mValue > 0) ? true : false;
}

/// Stream identifier
void Amplitude::setIdentifier(const StreamIdentifier &identifier)
{
    auto copy = identifier;
    setIdentifier(std::move(copy));
}

void Amplitude::setIdentifier(StreamIdentifier &&identifier)
{
    if (!identifier.hasNetwork())
    {
        throw std::invalid_argument("Network not set");
    }
    if (!identifier.hasStation())
    {
        throw std::invalid_argument("Station not set");
    }
    if (!identifier.hasChannel())
    {
        throw std::invalid_argument("Channel not set");
    }
    if (!identifier.hasLocationCode())
    {
        throw std::invalid_argument("Location code not set");
    }
    pImpl->mIdentifier = std::move(identifier);
    pImpl->mHasIdentifier = true;
}

StreamIdentifier Amplitude::getIdentifier() const
{
    if (!hasIdentifier())
    {
        throw std::runtime_error("Stream identifier not set");
    }
    return pImpl->mIdentifier;
}

bool Amplitude::hasIdentifier() const noexcept
{
    return pImpl->mHasIdentifier;
}

/// The stream identifier name
std::string Amplitude::getName() const
{
    if (!hasIdentifier())
    {
        throw std::runtime_error("Stream identifier not set");
    }
    return pImpl->mIdentifier.toString();
}

/// Protobuf
template<>
ULocalMagnitudeServiceAPI::V1::Magnitude::Amplitude
Amplitude::toMessage() const
{
    ULocalMagnitudeServiceAPI::V1::Magnitude::Amplitude result; 
    auto streamIdentifier
        = getIdentifier().toMessage
          <
              ULocalMagnitudeServiceAPI::V1::Magnitude::StreamIdentifier
          > ();
    *result.mutable_stream_identifier() = std::move(streamIdentifier);
    result.set_value(getValue());
    result.set_units(
       ULocalMagnitudeServiceAPI::V1::Magnitude::Amplitude_Units_MILLIMETERS);
    return result;
}

