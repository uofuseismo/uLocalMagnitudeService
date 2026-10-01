#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include "uLocalMagnitudeService/magnitude/stationMagnitudeCalculator.hpp"
#include "uLocalMagnitudeService/magnitude/stationMagnitude.hpp"
#include "uLocalMagnitudeService/magnitude/amplitude.hpp"
#include "uLocalMagnitudeService/magnitude/observation.hpp"
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"
#include "uLocalMagnitudeService/corrections/distance.hpp"
#include "uLocalMagnitudeService/corrections/distanceOptions.hpp"
#include "uLocalMagnitudeService/corrections/station.hpp"
#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"

using namespace ULocalMagnitudeService::Magnitude;

class StationMagnitudeCalculator::StationMagnitudeCalculatorImpl
{
public:
    StationMagnitudeCalculatorImpl(const Corrections::Station &station,
                const Corrections::Distance &distance) :
        mStationCorrection(station),
        mDistanceCorrection(distance)
    {
    }
    Corrections::Station mStationCorrection;
    Corrections::Distance mDistanceCorrection;
    bool mInitialized{false};
};

/// Constructor
StationMagnitudeCalculator::StationMagnitudeCalculator(
    const Corrections::Station &station,
    const Corrections::Distance &distance)
{
    if (!station.isInitialized())
    {
        throw std::invalid_argument("Station correction not initialized");
    }
    if (!distance.isInitialized())
    {
        throw std::invalid_argument("Distance correction not initialized");
    }
    pImpl = std::make_unique<StationMagnitudeCalculatorImpl>
            (station, distance);
    pImpl->mInitialized = true;
}

/// Copy constructor
StationMagnitudeCalculator::StationMagnitudeCalculator(
    const StationMagnitudeCalculator &station)
{
    *this = station;
}

/// Move constructor
StationMagnitudeCalculator::StationMagnitudeCalculator(
    StationMagnitudeCalculator &&station) noexcept
{
    *this = std::move(station);
}

/// Destructor
StationMagnitudeCalculator::~StationMagnitudeCalculator() = default;

/// Copy assignent
StationMagnitudeCalculator&
StationMagnitudeCalculator::operator=(const StationMagnitudeCalculator &station)
{
    if (&station == this){return *this;}
    pImpl = std::make_unique<StationMagnitudeCalculatorImpl> (*station.pImpl);
    return *this;
}

/// Move assignent
StationMagnitudeCalculator&
StationMagnitudeCalculator::operator=(StationMagnitudeCalculator &&station) noexcept
{
    if (&station == this){return *this;}
    pImpl = std::move(station.pImpl);
    return *this;
}

/// Initialized?
bool StationMagnitudeCalculator::isInitialized() const noexcept
{
    return pImpl->mInitialized;
}

/// Finally, do something science-y and compute a station magnitude
StationMagnitude
StationMagnitudeCalculator::operator()(const Observation &observation) const
{
    if (!isInitialized())
    {   
        throw std::runtime_error("Station magnitude calculator not initialized");
    }   
    if (!observation.hasAmplitudes())
    {
        throw std::invalid_argument("Amplitudes not set");
    }
    if (!observation.hasEpicentralDistance())
    {
        throw std::invalid_argument("Epicentral distance not set");
    }
    // Does this observation correspond to this station correction?
    auto stationName = observation.getStationName();
    auto correctionStationName = pImpl->mStationCorrection.getName();
    if (stationName != correctionStationName)
    {   
        throw std::invalid_argument(
             "Amplitude observations from "
           + stationName
           + " does not match this correction "
           + correctionStationName);
    }
    auto epicentralDistance = observation.getEpicentralDistance();
    double eventDepth{0};
    if (pImpl->mDistanceCorrection.getDistanceType() ==
        Corrections::DistanceOptions::Type::Hypocentral)
    {
        if (!observation.hasDepth())
        {
            throw std::invalid_argument("Event depth not set on observation");
        }
        eventDepth = observation.getDepth();
    }
    // Apply the station magnitude formula:
    //  log10(AvgAmp/2) + C_d + C_s
    const auto &amplitudes = observation.getAmplitudesReference();
    auto amplitude1 = amplitudes.first.getValue();
    auto amplitude2 = amplitudes.second.getValue();
    auto averageAmplitude = 0.5*(amplitude1 + amplitude2);
    auto distanceCorrection
        = getDistanceCorrection(epicentralDistance, eventDepth);
    auto stationCorrection = getStationCorrection();
    auto uncorrectedStationMagnitude = std::log10(0.5*averageAmplitude);
    auto stationMagnitude
        = uncorrectedStationMagnitude + distanceCorrection + stationCorrection;
    // Package up the output
    StationMagnitude result;
    result.setStationName(stationName);
    result.setValue(stationMagnitude);
    result.setStationCorrection(stationCorrection);
    result.setDistanceCorrection(distanceCorrection);
    return result;
}

double StationMagnitudeCalculator::getDistanceCorrection(
    const double epicentralDistance,
    const double eventDepth) const
{
    if (!isInitialized())
    {   
        throw std::runtime_error("Station magnitude calculator not initialized");
    } 
    if (epicentralDistance < 0)
    {   
        throw std::invalid_argument("Distance must be positive");
    }   
    return pImpl->mDistanceCorrection.operator()(epicentralDistance,
                                                 eventDepth);
}

double StationMagnitudeCalculator::getStationCorrection() const
{
    if (!isInitialized())
    {
        throw std::runtime_error("Station magnitude calculator not initialized");
    }
    return pImpl->mStationCorrection.operator()();
}
