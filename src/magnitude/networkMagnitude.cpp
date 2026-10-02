#include <memory>
#include <stdexcept>
#include <utility>
#include "uLocalMagnitudeService/magnitude/networkMagnitude.hpp"


using namespace ULocalMagnitudeService::Magnitude;

class NetworkMagnitude::NetworkMagnitudeImpl
{
public:
    double mValue{0};
    bool mHasValue{false};
};

/// Constructor
NetworkMagnitude::NetworkMagnitude() :
    pImpl(std::make_unique<NetworkMagnitudeImpl> ())
{
}

/// Copy constructor
NetworkMagnitude::NetworkMagnitude(const NetworkMagnitude &magnitude)
{
    *this = magnitude;
}

/// Move constructor
NetworkMagnitude::NetworkMagnitude(NetworkMagnitude &&magnitude) noexcept
{
    *this = std::move(magnitude);
}

/// Copy assignment
NetworkMagnitude& NetworkMagnitude::operator=(const NetworkMagnitude &magnitude)
{
    if (&magnitude == this){return *this;}
    pImpl = std::make_unique<NetworkMagnitudeImpl> (*magnitude.pImpl);
    return *this;
}

/// Move assignment
NetworkMagnitude& 
NetworkMagnitude::operator=(NetworkMagnitude &&magnitude) noexcept
{
    if (&magnitude == this){return *this;}
    pImpl = std::move(magnitude.pImpl);
    return *this;
}

/// Destructor
NetworkMagnitude::~NetworkMagnitude() = default;

/// Value
void NetworkMagnitude::setValue(const double value)
{
    if (value <-10 || value > 10)
    {
        throw std::invalid_argument("Magnitude must be in range [-10, 10]");
    }
    pImpl->mValue = value;
    pImpl->mHasValue = true;
}

double NetworkMagnitude::getValue() const
{
    if (!hasValue())
    {
        throw std::invalid_argument("Value not set");
    }
    return pImpl->mValue;
}

bool NetworkMagnitude::hasValue() const noexcept
{
    return pImpl->mHasValue;
}
