#include <cmath>
#include <memory>
#include <stdexcept>
#include <utility>
#include "uLocalMagnitudeService/corrections/stationLocation.hpp"
//#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/station_location.pb.h"

using namespace ULocalMagnitudeService::Corrections;

class StationLocation::StationLocationImpl
{
public:
    //StationIdentifier mIdentifier;
    double mLatitude{0};
    double mLongitude{0};
    //double mElevation{0};  
    //bool mHasIdentifier{false};
    bool mHasLatitude{false};
    bool mHasLongitude{false};
};

/// Constructor
StationLocation::StationLocation() :
    pImpl(std::make_unique<StationLocationImpl> ())
{
}

/// Copy constructor
StationLocation::StationLocation(const StationLocation &location)
{
    *this = location;
}

/// Move constructor
StationLocation::StationLocation(StationLocation &&location) noexcept
{
    *this = std::move(location);
}

/// Constructor
template<>
StationLocation::StationLocation(
    const ULocalMagnitudeServiceAPI::V1::Magnitude::StationLocation &location)
{
    if (!location.has_latitude())
    {   
        throw std::invalid_argument("Latitude not set");
    }   
    if (!location.has_longitude())
    {   
        throw std::invalid_argument("Longitude not set");
    }   
    StationLocation thisLocation;
    thisLocation.setLatitude(location.latitude());
    thisLocation.setLongitude(location.longitude());
    //thisLocation.setElevation(location.elevation());
    *this = std::move(thisLocation);
}


/// Destructor
StationLocation::~StationLocation() = default;

/// Copy assignment
StationLocation& StationLocation::operator=(const StationLocation &location)
{
    if (&location == this){return *this;}
    pImpl = std::make_unique<StationLocationImpl> (*location.pImpl);
    return *this;
}

/// Move assignment
StationLocation& StationLocation::operator=(StationLocation &&location) noexcept
{
    if (&location == this){return *this;}
    pImpl = std::move(location.pImpl);
    return *this;
}

/// Latitude
void StationLocation::setLatitude(const double latitude)
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

double StationLocation::getLatitude() const
{
    if (!hasLatitude()){throw std::runtime_error("Latitude not set");}
    return pImpl->mLatitude; 
}   

bool StationLocation::hasLatitude() const noexcept
{
    return pImpl->mHasLatitude;
}   

/// Longitude
void StationLocation::setLongitude(const double longitude)
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

double StationLocation::getLongitude() const
{
    if (!hasLongitude()){throw std::runtime_error("Longitude not set");}
    return pImpl->mLongitude;
}

bool StationLocation::hasLongitude() const noexcept
{
    return pImpl->mHasLongitude;
}

/// Elevation
/*
void StationLocation::setElevation(const double elevation)
{
    if (!std::isfinite(elevation))
    {   
        throw std::invalid_argument("Elevation is not finite");
    }   
    if (elevation < -10000 || elevation > 8600)
    {
        throw std::invalid_argument("Elevation must be in range [-10000, 8600] meters");
    }
    pImpl->mElevation = elevation;
    pImpl->mHasElevation = true;
}

double StationLocation::getElevation() const
{
    if (!hasElevation()){throw std::runtime_error("Elevation not set");}
    return pImpl->mElevation;
}

bool StationLocation::hasElevation() const noexcept
{
    return pImpl->mHasElevation;
}
*/

/// Station identifier
/*
void StationLocation::setIdentifier(const StationIdentifier &identifier)
{
    if (!identifier.hasNetwork())
    {
        throw std::invalid_argument("Network not set");
    }
    if (!identifier.hasStation())
    {
        throw std::invalid_argument("Station not set");
    }
    pImpl->mIdentifier = identifier;
    pImpl->mHasIdentifier = true;
}
    

StationIdentifier StationLocation::getIdentifier() const
{
    if (!hasIdentifier())
    {
        throw std::runtime_error("Station identifier does not exist");
    }
    return pImpl->mIdentifier;
}

bool StationLocation::hasIdentifier() const noexcept
{
   return pImpl->mHasIdentifier;
}
*/
