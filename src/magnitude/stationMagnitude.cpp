#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include "uLocalMagnitudeService/magnitude/stationMagnitude.hpp"
#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"
#include "uLocalMagnitudeService/magnitude/amplitude.hpp"
#include "uLocalMagnitudeService/magnitude/observation.hpp"
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"
//#include "uLocalMagnitudeServiceAPI/v1/magnitude/station_magnitude.pb.h"

using namespace ULocalMagnitudeService::Magnitude;

class StationMagnitude::StationMagnitudeImpl
{
public:
    std::pair<Amplitude, Amplitude> mAmplitudes;
    std::string mStationName;
    double mValue{0};
    double mStationCorrection{0};
    double mDistanceCorrection{0};
    bool mHasAmplitudes{false};
    bool mHasValue{false};
    bool mHasStationCorrection{false};
    bool mHasDistanceCorrection{false};
};

/// Constructor
StationMagnitude::StationMagnitude() :
    pImpl(std::make_unique<StationMagnitudeImpl> ())
{
}

/// Copy constructor
StationMagnitude::StationMagnitude(const StationMagnitude &magnitude)
{
    *this = magnitude;
}

/// Move constructor
StationMagnitude::StationMagnitude(StationMagnitude &&magnitude) noexcept
{
    *this = std::move(magnitude);
}

/// Copy assignment
StationMagnitude&
StationMagnitude::operator=(const StationMagnitude &magnitude)
{
    if (&magnitude == this){return *this;}
    pImpl = std::make_unique<StationMagnitudeImpl> (*magnitude.pImpl);
    return *this;
}

/// Move assignment
StationMagnitude&
StationMagnitude::operator=(StationMagnitude &&magnitude) noexcept
{
    if (&magnitude == this){return *this;}
    pImpl = std::move(magnitude.pImpl);
    return *this;
}

/// Destructor
StationMagnitude::~StationMagnitude() = default;

/// Value
void StationMagnitude::setValue(const double value)
{
    if (!std::isfinite(value))
    {
        throw std::invalid_argument("Station magnitude must be finite");
    }
    pImpl->mValue = value;
    pImpl->mHasValue = true;
}

double StationMagnitude::getValue() const
{
    if (!hasValue()){throw std::runtime_error("Station magnitude not set");}
    return pImpl->mValue;
}

bool StationMagnitude::hasValue() const noexcept
{
    return pImpl->mHasValue;
}

/// Station correction
void StationMagnitude::setStationCorrection(const double correction)
{
    if (!std::isfinite(correction))
    {
        throw std::invalid_argument("Station correction must be finite");
    }
    pImpl->mStationCorrection = correction;
    pImpl->mHasStationCorrection = true;
}

double StationMagnitude::getStationCorrection() const
{
    if (!hasStationCorrection())
    {
        throw std::runtime_error("Station correction not set");
    }
    return pImpl->mStationCorrection;
}

bool StationMagnitude::hasStationCorrection() const noexcept
{
    return pImpl->mHasStationCorrection;
}

/// Distance correction
void StationMagnitude::setDistanceCorrection(const double correction)
{
    if (!std::isfinite(correction))
    {
        throw std::invalid_argument("Distance correction must be finite");
    }
    pImpl->mDistanceCorrection = correction;
    pImpl->mHasDistanceCorrection = true;
}

double StationMagnitude::getDistanceCorrection() const
{
    if (!hasDistanceCorrection())
    {
        throw std::runtime_error("Distance correction not set");
    }
    return pImpl->mDistanceCorrection;
}

bool StationMagnitude::hasDistanceCorrection() const noexcept
{
    return pImpl->mHasDistanceCorrection;
}

/// Station name
std::string StationMagnitude::getStationName() const
{
    if (!hasAmplitudes()){throw std::runtime_error("Amplitudes not set");}
    return pImpl->mStationName;
}

/// Amplitudes
void StationMagnitude::setAmplitudes(
    const std::pair<Amplitude, Amplitude> &amplitudes)
{
    Observation observation;
    observation.setAmplitudes(amplitudes); // Handle error checking
    pImpl->mAmplitudes = observation.getAmplitudes();
    const StreamIdentifier streamIdentifier
    {
        pImpl->mAmplitudes.first.getIdentifier()
    };
    const Corrections::StationIdentifier stationIdentifier
    {
        streamIdentifier
    };  
    pImpl->mStationName = stationIdentifier.toString();
    pImpl->mHasAmplitudes = true;
}

std::pair<Amplitude, Amplitude>
StationMagnitude::getAmplitudes() const
{
    if (!hasAmplitudes())
    {
        throw std::runtime_error("Amplitudes not set");
    }
    return pImpl->mAmplitudes;
} 

bool StationMagnitude::hasAmplitudes() const noexcept
{
    return pImpl->mHasAmplitudes;
}
