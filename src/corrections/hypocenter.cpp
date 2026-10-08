#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include "uLocalMagnitudeService/corrections/hypocenter.hpp"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/hypocenter.pb.h"

using namespace ULocalMagnitudeService::Corrections;

class Hypocenter::HypocenterImpl
{
public:
    double mLatitude{0};
    double mLongitude{0};
    double mDepth{0};  
    bool mHasLatitude{false};
    bool mHasLongitude{false};
    bool mHasDepth{false};
};

/// Constructor
Hypocenter::Hypocenter() :
    pImpl(std::make_unique<HypocenterImpl> ())
{
}

/// Copy constructor
Hypocenter::Hypocenter(const Hypocenter &origin)
{
    *this = origin;
}

/// Move constructor
Hypocenter::Hypocenter(Hypocenter &&origin) noexcept
{
    *this = std::move(origin);
}

/// Destructor
Hypocenter::~Hypocenter() = default;

/// Copy assignment
Hypocenter& Hypocenter::operator=(const Hypocenter &location)
{
    if (&location == this){return *this;}
    pImpl = std::make_unique<HypocenterImpl> (*location.pImpl);
    return *this;
}

/// Move assignment
Hypocenter& Hypocenter::operator=(Hypocenter &&location) noexcept
{
    if (&location == this){return *this;}
    pImpl = std::move(location.pImpl);
    return *this;
}

/// Constructor
template<>
Hypocenter::Hypocenter(
    const ULocalMagnitudeServiceAPI::V1::Magnitude::Hypocenter &hypocenter)
{
    if (!hypocenter.has_depth())
    {
        throw std::invalid_argument("Depth not set");
    }
    if (!hypocenter.has_latitude())
    {
        throw std::invalid_argument("Latitude not set");
    }
    if (!hypocenter.has_longitude())
    {
        throw std::invalid_argument("Longitude not set");
    }
    Hypocenter thisHypocenter;
    thisHypocenter.setLatitude(hypocenter.latitude());
    thisHypocenter.setLongitude(hypocenter.longitude());
    thisHypocenter.setDepth(hypocenter.depth());
    *this = std::move(thisHypocenter);
}

/// Latitude
void Hypocenter::setLatitude(const double latitude)
{
    if (!std::isfinite(latitude))
    {
        throw std::invalid_argument("Latitude is not finite");
    }
    if (latitude < -90 || latitude > 90)
    {
        throw std::invalid_argument("Latitude must be in range [-90, 90]");
    }
    pImpl->mLatitude = latitude;
    pImpl->mHasLatitude = true;
}

double Hypocenter::getLatitude() const
{
    if (!hasLatitude()){throw std::runtime_error("Latitude not set");}
    return pImpl->mLatitude; 
}   

bool Hypocenter::hasLatitude() const noexcept
{
    return pImpl->mHasLatitude;
}   

/// Longitude
void Hypocenter::setLongitude(const double longitude)
{
    if (!std::isfinite(longitude))
    {
        throw std::invalid_argument("Longitude is not finite");
    }
    auto lon = std::fmod(longitude, 360.0);
    if (lon < 0){lon = lon + 360.0;}
    pImpl->mLongitude = lon;
    pImpl->mHasLongitude = true;
}

double Hypocenter::getLongitude() const
{
    if (!hasLongitude()){throw std::runtime_error("Longitude not set");}
    return pImpl->mLongitude;
}

bool Hypocenter::hasLongitude() const noexcept
{
    return pImpl->mHasLongitude;
}

/// Depth
void Hypocenter::setDepth(const double depth)
{
    if (!std::isfinite(depth))
    {
        throw std::invalid_argument("Depth is not finite");
    }   
    constexpr double minimumDepth{-8600};
    constexpr double maximumDepth{900000};
    if (depth < minimumDepth || depth > maximumDepth)
    {
        throw std::invalid_argument("Depth must be in range ["
                                  + std::to_string(minimumDepth) + ","
                                  + std::to_string(maximumDepth) + "] meters");
    }
    pImpl->mDepth = depth;
    pImpl->mHasDepth = true;
}

double Hypocenter::getDepth() const
{
    if (!hasDepth()){throw std::runtime_error("Depth not set");}
    return pImpl->mDepth;
}

bool Hypocenter::hasDepth() const noexcept
{
    return pImpl->mHasDepth;
}
