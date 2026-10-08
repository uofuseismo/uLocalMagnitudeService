#include <memory>
#include <stdexcept>
#include <utility>
#include "uLocalMagnitudeService/magnitude/residual.hpp"

using namespace ULocalMagnitudeService::Magnitude;

class Residual::ResidualImpl
{
public:
    double mValue{0};
    bool mHasValue{false};
}; 

/// Constructor
Residual::Residual() :
    pImpl(std::make_unique<ResidualImpl> ())
{
}

/// Copy constructor
Residual::Residual(const Residual &residual)
{
    *this = residual;
}

/// Move constructor
Residual::Residual(Residual &&residual) noexcept
{
    *this = std::move(residual);
}

/// Copy assignment
Residual& Residual::operator=(const Residual &residual)
{
    if (&residual == this){return *this;}
    pImpl = std::make_unique<ResidualImpl> (*residual.pImpl);
    return *this;
}

/// Move assignment
Residual& Residual::operator=(Residual &&residual) noexcept
{
    if (&residual == this){return *this;}
    pImpl = std::move(residual.pImpl);
    return *this;
}

/// Destructor
Residual::~Residual() = default;

/// Value
void Residual::setValue(const double value) noexcept
{
    pImpl->mValue = value;
    pImpl->mHasValue = true;
}

double Residual::getValue() const
{
    if (!hasValue())
    {
        throw std::runtime_error("Residual value not set");
    }
    return pImpl->mValue;
}

bool Residual::hasValue() const noexcept
{
    return pImpl->mHasValue;
}


