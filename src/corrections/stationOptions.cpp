#include <memory>
#include <stdexcept>
#include <utility>
#include "uLocalMagnitudeService/corrections/stationOptions.hpp"
#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"

using namespace ULocalMagnitudeService::Corrections;

class StationOptions::StationOptionsImpl
{
public:
    StationIdentifier mIdentifier;
    double mCorrection{0};
    bool mHasIdentifier{false};
    bool mHasCorrection{false};
};

/// Constructor
StationOptions::StationOptions() :
    pImpl(std::make_unique<StationOptionsImpl> ())
{
}

/// Copy constructor
StationOptions::StationOptions(const StationOptions &options)
{
    *this = options;
}

/// Move constructor
StationOptions::StationOptions(StationOptions &&options) noexcept
{
    *this = std::move(options);
}

/// Destructor
StationOptions::~StationOptions() = default;

/// Copy assignent
StationOptions& StationOptions::operator=(const StationOptions &options)
{
    if (&options == this){return *this;}
    pImpl = std::make_unique<StationOptionsImpl> (*options.pImpl);
    return *this;
}

/// Move assignent
StationOptions& StationOptions::operator=(StationOptions &&options) noexcept
{
    if (&options == this){return *this;}
    pImpl = std::move(options.pImpl);
    return *this;
}

/// Identifier
void StationOptions::setIdentifier(const StationIdentifier &identifier)
{
    if (!identifier.hasNetwork())
    {
        throw std::invalid_argument("Network not set on identifier");
    }
    if (!identifier.hasStation())
    {
        throw std::invalid_argument("Station not set on identifier");
    }
    pImpl->mIdentifier = identifier;
    pImpl->mHasIdentifier = true;
}

StationIdentifier StationOptions::getIdentifier() const
{
    if (!hasIdentifier())
    {   
        throw std::runtime_error("Identifier not set");
    }
    return pImpl->mIdentifier;
}

bool StationOptions::hasIdentifier() const noexcept
{
    return pImpl->mHasIdentifier;
}
 

/// Correction
void StationOptions::setCorrection(const double correction) noexcept
{
    pImpl->mCorrection = correction;
    pImpl->mHasCorrection = true;
}

double StationOptions::getCorrection() const
{
    if (!hasCorrection())
    {
        throw std::runtime_error("Correction not set");
    }
    return pImpl->mCorrection;
}

bool StationOptions::hasCorrection() const noexcept
{
    return pImpl->mHasCorrection;
}
