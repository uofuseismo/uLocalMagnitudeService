#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include "uLocalMagnitudeService/magnitude/station.hpp"
#include "uLocalMagnitudeService/magnitude/amplitude.hpp"
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"
#include "uLocalMagnitudeService/corrections/distance.hpp"
#include "uLocalMagnitudeService/corrections/station.hpp"
#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"

using namespace ULocalMagnitudeService::Magnitude;

class Station::StationImpl
{
public:
    StationImpl(const Corrections::Station &station,
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
Station::Station(const Corrections::Station &station,
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
    pImpl = std::make_unique<StationImpl> (station, distance);
    pImpl->mInitialized = true;
}

/// Copy constructor
Station::Station(const Station &station)
{
    *this = station;
}

/// Move constructor
Station::Station(Station &&station) noexcept
{
    *this = std::move(station);
}

/// Destructor
Station::~Station() = default;

/// Copy assignent
Station& Station::operator=(const Station &station)
{
    if (&station == this){return *this;}
    pImpl = std::make_unique<StationImpl> (*station.pImpl);
    return *this;
}

/// Move assignent
Station& Station::operator=(Station &&station) noexcept
{
    if (&station == this){return *this;}
    pImpl = std::move(station.pImpl);
    return *this;
}

/// Initialized?
bool Station::isInitialized() const noexcept
{
    return pImpl->mInitialized;
}

/// Finally, do something science-y and compute a station magnitude
double Station::operator()(
    const std::pair<Amplitude, Amplitude> &amplitudes,
    const double epicentralDistance,
    const double eventDepth) const
{
    if (!isInitialized())
    {
        throw std::runtime_error("Station magnitude class not initialized");
    }
    if (!amplitudes.first.hasValue())
    {
        throw std::invalid_argument("No amplitude on first observation");
    }
    if (!amplitudes.second.hasValue())
    {
        throw std::invalid_argument("No amplitude on second observation");
    }
    if (!amplitudes.first.hasIdentifier())
    {   
        throw std::invalid_argument(
           "No stream identifier on first observation");
    }   
    if (!amplitudes.second.hasIdentifier())
    {   
        throw std::invalid_argument(
           "No stream identifier on second observation");
    }
    // Need a few things to match up based on NSCL
    auto streamIdentifier1 = amplitudes.first.getIdentifier();
    auto streamIdentifier2 = amplitudes.second.getIdentifier();
    if (streamIdentifier1.toString() == streamIdentifier2.toString())
    {
        throw std::invalid_argument("Amplitude observations from same channel");
    }
    // Now need to match the network/station to match (and this needs to match
    // our station correction)
    const Corrections::StationIdentifier stationIdentifier1{streamIdentifier1};
    const Corrections::StationIdentifier stationIdentifier2{streamIdentifier2};
    if (stationIdentifier1.toString() != stationIdentifier2.toString())
    {
        throw std::invalid_argument(
           "Amplitude observations made on different stations");
    }
    // Only need one check (transitive b/c stationId1 == stationId2
    if (stationIdentifier1.toString() != pImpl->mStationCorrection.getName())
    {
        throw std::invalid_argument(
             "Amplitude observations from "
           + stationIdentifier1.toString()
           + " does not match this correction "
           + pImpl->mStationCorrection.getName());
    }
    // Check the location code b/c it's easy
    if (streamIdentifier1.getLocationCode() != 
        streamIdentifier2.getLocationCode())
    {
        throw std::invalid_argument(
            "Amplitude observations made on different streams");
    }
    // Check everything but the last letter (component) on channel
    auto channel1 = streamIdentifier1.getChannel();
    auto channel2 = streamIdentifier2.getChannel();
    if (channel1.substr(0, channel1.size() - 1) !=
        channel2.substr(0, channel2.size() - 1))
    {
        throw std::invalid_argument(
            "Amplitude observations made on different components - "
           + channel1 + " " + channel2);
    }
    // Anti-climatic but finally apply the station magnitude formula:
    //  log10(AvgAmp/2) + C_d + C_s
    auto amplitude1 = amplitudes.first.getValue();
    auto amplitude2 = amplitudes.second.getValue();
    auto averageAmplitude = 0.5*(amplitude1 + amplitude2);
    auto correction = getDistanceCorrection(epicentralDistance, eventDepth)
                    + getStationCorrection();
    auto uncorrectedStationMagnitude = std::log10(0.5*averageAmplitude);
    return uncorrectedStationMagnitude + correction;
}

double Station::getDistanceCorrection(const double epicentralDistance,
                                      const double eventDepth) const
{
    if (!isInitialized())
    {   
        throw std::runtime_error("Station magnitude class not initialized");
    } 
    if (epicentralDistance < 0)
    {   
        throw std::invalid_argument("Distance must be positive");
    }   
    return pImpl->mDistanceCorrection.operator()(epicentralDistance,
                                                 eventDepth);
}

double Station::getStationCorrection() const
{
    if (!isInitialized())
    {
        throw std::runtime_error("Station magnitude class not initialized");
    }
    return pImpl->mStationCorrection.operator()();
}
