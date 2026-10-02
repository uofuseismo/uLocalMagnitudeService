#include <cmath>
#include <expected>
#include <memory>
#include <stdexcept>
#include <utility>
#include "uLocalMagnitudeService/magnitude/stationMagnitudeCalculator.hpp"
#include "uLocalMagnitudeService/magnitude/stationMagnitude.hpp"
#include "uLocalMagnitudeService/magnitude/amplitude.hpp"
#include "uLocalMagnitudeService/magnitude/observation.hpp"
#include "uLocalMagnitudeService/corrections/distance.hpp"
#include "uLocalMagnitudeService/corrections/distanceOptions.hpp"
#include "uLocalMagnitudeService/corrections/station.hpp"

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
std::expected<StationMagnitude, StationMagnitudeCalculator::ErrorCode> 
StationMagnitudeCalculator::operator()(
    const Observation &observation) const noexcept 
{
    if (!isInitialized())
    {   
        return std::unexpected(ErrorCode::Uninitialized);
        //throw std::runtime_error("Station magnitude calculator not initialized");
    }   
    if (!observation.hasAmplitudes())
    {
        return std::unexpected(ErrorCode::NoAmplitudes);
    }
    if (!observation.hasEpicentralDistance())
    {
        return std::unexpected(ErrorCode::NoEpicentralDistance);
    }
    // Does this observation correspond to this station correction?
    auto stationName = observation.getStationName();
    auto correctionStationName = pImpl->mStationCorrection.getName();
    if (stationName != correctionStationName)
    {
        return std::unexpected(ErrorCode::StationCorrectionMismatch);
    }
    auto epicentralDistance = observation.getEpicentralDistance();
    double eventDepth{0};
    if (pImpl->mDistanceCorrection.getDistanceType() ==
        Corrections::DistanceOptions::Type::Hypocentral)
    {
        if (!observation.hasDepth())
        {
            return std::unexpected(ErrorCode::NoEventDepth);
        }
        eventDepth = observation.getDepth();
    }
    // Apply the station magnitude formula:
    //  log10(AvgAmp/2) + C_d + C_s
    const auto &amplitudes = observation.getAmplitudesReference();
    auto amplitude1 = amplitudes.first.getValue();
    auto amplitude2 = amplitudes.second.getValue();
    auto averageAmplitude = 0.5*(amplitude1 + amplitude2);
    double distanceCorrection{0};
    auto distanceCorrectionResult
        = getDistanceCorrection(epicentralDistance, eventDepth);
    if (distanceCorrectionResult.has_value())
    {
        distanceCorrection = *distanceCorrectionResult;
    }
    else
    {
         return std::unexpected(distanceCorrectionResult.error());
    }
    double stationCorrection{0};
    auto stationCorrectionResult = getStationCorrection();
    if (stationCorrectionResult.has_value())
    {
        stationCorrection = *stationCorrectionResult;
    }
    else
    {
        return std::unexpected(stationCorrectionResult.error());
    }
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

std::expected<double, StationMagnitudeCalculator::ErrorCode>
StationMagnitudeCalculator::getDistanceCorrection(
    const double epicentralDistance,
    const double eventDepth) const noexcept
{
    if (!isInitialized())
    {
        return std::unexpected(ErrorCode::Uninitialized);
    }
    if (epicentralDistance < 0)
    {
        return std::unexpected(ErrorCode::NegativeDistance);
    }
    auto distanceCorrection
        =  pImpl->mDistanceCorrection.operator()(epicentralDistance,
                                                 eventDepth);

    if (distanceCorrection.has_value())
    {
        return *distanceCorrection;
    }
    else if (distanceCorrection.error() ==
             Corrections::Distance::ErrorCode::NegativeDistance)
    {
        return std::unexpected(ErrorCode::NegativeDistance);
    }
    else if (distanceCorrection.error() ==
             Corrections::Distance::ErrorCode::StationTooFar)
    {
        return std::unexpected(ErrorCode::StationTooFar);
    }
    else if (distanceCorrection.error() ==
             Corrections::Distance::ErrorCode::InvalidSourceDepth)
    {
        return std::unexpected(ErrorCode::InvalidDepth);
    }
    else if (distanceCorrection.error() ==
             Corrections::Distance::ErrorCode::Uninitialized)
    {
         return std::unexpected(ErrorCode::UnitializedDistanceCorrection);
    }
    else
    {
         return std::unexpected(ErrorCode::Algorithmic);
    }
}

std::expected<double, StationMagnitudeCalculator::ErrorCode> 
StationMagnitudeCalculator::getStationCorrection() const noexcept
{
    if (!isInitialized())
    {
        return std::unexpected(ErrorCode::Uninitialized);        
    }
    auto stationCorrectionResult = pImpl->mStationCorrection.operator()();;
    if (stationCorrectionResult.has_value())
    {
        return *stationCorrectionResult;
    }
    else if (stationCorrectionResult.error() ==
             Corrections::Station::ErrorCode::Uninitialized)
    {
        return std::unexpected(ErrorCode::UnitializedStationCorrection);
    }
    else
    {
        return std::unexpected(ErrorCode::Algorithmic);
    }
}
