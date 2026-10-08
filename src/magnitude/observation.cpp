#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include "uLocalMagnitudeService/magnitude/observation.hpp"
#include "uLocalMagnitudeService/corrections/distance.hpp"
#include "uLocalMagnitudeService/corrections/hypocenter.hpp"
#include "uLocalMagnitudeService/corrections/stationLocation.hpp"
#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"
#include "uLocalMagnitudeService/magnitude/amplitude.hpp"
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/hypocenter.pb.h"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/station_amplitude_measurement.pb.h"
//#include "uLocalMagnitudeServiceAPI/v1/magnitude/amplitude.pb.h"

namespace
{
constexpr double MIN_DEPTH{-8600};
constexpr double MAX_DEPTH{900000};
}

using namespace ULocalMagnitudeService::Magnitude;

class Observation::ObservationImpl
{
public:
    std::pair<Amplitude, Amplitude> mAmplitudes;
    std::string mStationName;
    double mEpicentralDistance{0};
    double mSourceDepth{0};
    bool mHasAmplitudes{false};
    bool mHasEpicentralDistance{false};
    bool mHasSourceDepth{false};
};

/// Constructor
Observation::Observation() :
    pImpl(std::make_unique<ObservationImpl> ())
{
}

/// Copy constructor
Observation::Observation(const Observation &observation)
{
    *this = observation;
}

/// Move constructor
Observation::Observation(Observation &&observation) noexcept
{
    *this = std::move(observation);
}

/// Create from a station amplitude measurement
template<>
Observation::Observation(
    const ULocalMagnitudeServiceAPI::V1
          ::Magnitude::StationAmplitudeMeasurement &measurement,
    const ULocalMagnitudeServiceAPI::V1
          ::Magnitude::Hypocenter &hypocenterMessage)
{
    const Corrections::Hypocenter hypocenter{hypocenterMessage};
    if (!measurement.has_amplitude_stream_1())
    {
        throw std::invalid_argument("First amplitude not set");
    }
    if (!measurement.has_amplitude_stream_2())
    {
        throw std::invalid_argument("Second amplitude not set");
    }
    if (!measurement.has_station_location())
    {
        throw std::invalid_argument("Station location not set");
    }
    const Corrections::StationLocation stationLocation
    {
         measurement.station_location()
    };
    auto epicentralDistance
        = Corrections::Distance::computeEpicentralDistance(
             hypocenter, stationLocation);
    Observation thisObservation;
    Amplitude amplitude1{measurement.amplitude_stream_1()};  
    Amplitude amplitude2{measurement.amplitude_stream_2()};
    std::pair<Amplitude, Amplitude> amplitudePair
    {
        std::move(amplitude1),
        std::move(amplitude2)
    };
    thisObservation.setAmplitudes(std::move(amplitudePair));
    thisObservation.setEpicentralDistance(epicentralDistance);
    thisObservation.setDepth(hypocenter.getDepth());
    *this = std::move(thisObservation);
}

/// Copy assignment
Observation &Observation::operator=(const Observation &observation)
{
    if (&observation == this){return *this;}
    pImpl = std::make_unique<ObservationImpl> (*observation.pImpl);
    return *this;
}

/// Move assignment
Observation &Observation::operator=(Observation &&observation) noexcept
{
    if (&observation == this){return *this;}
    pImpl = std::move(observation.pImpl);
    return *this;
}

/// Destructor
Observation::~Observation() = default;

void Observation::setAmplitudes(
    const std::pair<Amplitude, Amplitude> &amplitudes)
{
    auto copy = amplitudes;
    setAmplitudes(std::move(copy));
}

void Observation::setAmplitudes(
    std::pair<Amplitude, Amplitude> &&amplitudes)
{
    if (!amplitudes.first.hasValue())
    {   
        throw std::invalid_argument("No amplitude on first amplitude");
    }   
    if (!amplitudes.second.hasValue())
    {   
        throw std::invalid_argument("No amplitude on second amplitude");
    }   
    if (!amplitudes.first.hasIdentifier())
    {   
        throw std::invalid_argument(
           "No stream identifier on first amplitude");
    }   
    if (!amplitudes.second.hasIdentifier())
    {   
        throw std::invalid_argument(
           "No stream identifier on second amplitude");
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
    pImpl->mStationName = stationIdentifier1.toString(); 
    pImpl->mAmplitudes = std::move(amplitudes);
    pImpl->mHasAmplitudes = true;
}

std::pair<Amplitude, Amplitude> Observation::getAmplitudes() const
{
    if (!hasAmplitudes())
    {
        throw std::runtime_error("Amplitudes not set");
    }
    return pImpl->mAmplitudes;
}

const std::pair<Amplitude, Amplitude> &
    Observation::getAmplitudesReference() const
{
    if (!hasAmplitudes())
    {
        throw std::runtime_error("Amplitudes not set");
    }
    return pImpl->mAmplitudes;
}

std::string Observation::getStationName() const
{
    if (!hasAmplitudes())
    {
        throw std::runtime_error("Amplitudes not set");
    }
    return pImpl->mStationName;
}

bool Observation::hasAmplitudes() const noexcept
{
    return pImpl->mHasAmplitudes;
}

/// Source-receiver distance
void Observation::setEpicentralDistance(const double distance)
{
    if (!std::isfinite(distance))
    {
        throw std::invalid_argument("Distance must be finite");
    }
    if (distance < 0)
    {
        throw std::invalid_argument("Epicentral distance must be non-negative");
    }
    if (distance > 21000000)
    {
        throw std::invalid_argument(
            "Epicentral Distance cannot exceed 21,000,000");
    }
    pImpl->mEpicentralDistance = distance;
    pImpl->mHasEpicentralDistance = true;
}

double Observation::getEpicentralDistance() const
{
    if (!hasEpicentralDistance()) 
    {
        throw std::runtime_error("Epicentral distance not set");
    }
    return pImpl->mEpicentralDistance;
}

bool Observation::hasEpicentralDistance() const noexcept
{
    return pImpl->mHasEpicentralDistance;
}

/// Source depth
void Observation::setDepth(const double depth)
{
    if (!std::isfinite(depth))
    {   
        throw std::invalid_argument("Depth must be finite");
    }   
    if (depth < MIN_DEPTH || depth > MAX_DEPTH)
    {
        throw std::invalid_argument("Depth must be in range [-8600, 900000]");
    }
    pImpl->mSourceDepth = depth;
    pImpl->mHasSourceDepth = true;
}

double Observation::getDepth() const
{
    if (!hasDepth())
    {
        throw std::runtime_error("Depth not set");
    }
    return pImpl->mSourceDepth;
}

bool Observation::hasDepth() const noexcept
{
    return pImpl->mHasSourceDepth;
}
