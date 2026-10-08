#include <chrono>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include "uLocalMagnitudeService/corrections/origin.hpp"

using namespace ULocalMagnitudeService::Corrections;

class Origin::OriginImpl
{
public:
    std::chrono::nanoseconds mTime;
    double mLatitude{0};
    double mLongitude{0};
    double mDepth{0};  
    bool mHasTime{false};
    bool mHasLatitude{false};
    bool mHasLongitude{false};
    bool mHasDepth{false};
};

/// Constructor
Origin::Origin() :
    pImpl(std::make_unique<OriginImpl> ())
{
}

/// Destructor
Origin::~Origin() = default;

/// Copy assignment
Origin& Origin::operator=(const Origin &location)
{
    if (&location == this){return *this;}
    pImpl = std::make_unique<OriginImpl> (*location.pImpl);
    return *this;
}

/// Move assignment
Origin& Origin::operator=(Origin &&location) noexcept
{
    if (&location == this){return *this;}
    pImpl = std::move(location.pImpl);
    return *this;
}

/// Latitude
void Origin::setLatitude(const double latitude)
{
    if (latitude < -90 || latitude > 90)
    {
        throw std::invalid_argument("Latitude must be in range [-90, 90]");
    }
    pImpl->mLatitude = latitude;
    pImpl->mHasLatitude = true;
}

double Origin::getLatitude() const
{
    if (!hasLatitude()){throw std::runtime_error("Latitude not set");}
    return pImpl->mLatitude; 
}   

bool Origin::hasLatitude() const noexcept
{
    return pImpl->mHasLatitude;
}   

/// Longitude
void Origin::setLongitude(const double longitude) noexcept
{
    auto lon = std::fmod(longitude, 360.0);
    if (lon < 0){lon = lon + 360.0;}
    pImpl->mLongitude = lon;
    pImpl->mHasLongitude = true;
}

double Origin::getLongitude() const
{
    if (!hasLongitude()){throw std::runtime_error("Longitude not set");}
    return pImpl->mLongitude;
}

bool Origin::hasLongitude() const noexcept
{
    return pImpl->mHasLongitude;
}

/// Depth
void Origin::setDepth(const double depth)
{
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

double Origin::getDepth() const
{
    if (!hasDepth()){throw std::runtime_error("Depth not set");}
    return pImpl->mDepth;
}

bool Origin::hasDepth() const noexcept
{
    return pImpl->mHasDepth;
}

/// Time
void Origin::setTime(const std::chrono::nanoseconds &time) noexcept
{
    pImpl->mTime = time;
    pImpl->mHasTime = true;
}

std::chrono::nanoseconds Origin::getTime() const
{
    if (!hasTime()){throw std::runtime_error("Origin time not set");}
    return pImpl->mTime;
}

bool Origin::hasTime() const noexcept
{
    return pImpl->mHasTime;
}

