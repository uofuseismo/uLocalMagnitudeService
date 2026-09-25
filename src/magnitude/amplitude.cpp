#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include "uLocalMagnitudeService/magnitude/amplitude.hpp"
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"

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
