#include <memory>
#include <stdexcept>
#include <utility>
#include "uLocalMagnitudeService/magnitude/residual.hpp"
#include "uLocalMagnitudeService/magnitude/stationMagnitude.hpp"
#include "uLocalMagnitudeService/magnitude/amplitude.hpp"
// NOLINTNEXTLINE(misc-include-cleaner)
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"
#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"
#include <uLocalMagnitudeServiceAPI/magnitude/v1/network_magnitude_from_amplitudes_response.pb.h>
#include <uLocalMagnitudeServiceAPI/magnitude/v1/station_magnitude.pb.h>
#include <uLocalMagnitudeServiceAPI/magnitude/v1/station_identifier.pb.h>

namespace UAPIV1 = ULocalMagnitudeServiceAPI::Magnitude::V1;
using namespace ULocalMagnitudeService::Magnitude;

class Residual::ResidualImpl
{
public:
    StationMagnitude mStationMagnitude;
    double mValue{0};
    bool mHasValue{false};
    bool mHasStationMagnitude{false};
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

/// Station magnitude
void Residual::setStationMagnitude(StationMagnitude &&stationMagnitude)
{
    if (!stationMagnitude.hasValue())
    {
        throw std::invalid_argument("Station magnitude not set");
    }
    if (!stationMagnitude.hasAmplitudes())
    {
        throw std::invalid_argument("Amplitudes not set");
    }
    if (!stationMagnitude.hasDistanceCorrection())
    {
        throw std::invalid_argument("Distance correction not set");
    }
    if (!stationMagnitude.hasStationCorrection())
    {
        throw std::invalid_argument("Station correction not set");
    }
    pImpl->mStationMagnitude = std::move(stationMagnitude);
    pImpl->mHasStationMagnitude = true;
}

void Residual::setStationMagnitude(const StationMagnitude &stationMagnitude)
{
    StationMagnitude copy{stationMagnitude};
    setStationMagnitude(std::move(copy));
}

StationMagnitude Residual::getStationMagnitude() const
{
    if (!hasStationMagnitude())
    {
        throw std::runtime_error("Station magnitude not set");
    }
    return pImpl->mStationMagnitude;
}

const StationMagnitude &Residual::getStationMagnitudeReference() const
{
    if (!hasStationMagnitude())
    {
        throw std::runtime_error("Station magnitude not set");
    }
    return *&pImpl->mStationMagnitude;
}


bool Residual::hasStationMagnitude() const noexcept
{
    return pImpl->mHasStationMagnitude;
}

template<>
UAPIV1::NetworkMagnitudeFromAmplitudesResponse::StationMagnitudeAndResidual
Residual::toStationMagnitudeAndResidualMessage() const
{
    if (!hasStationMagnitude())
    {
        throw std::runtime_error("Station magnitude not set");
    }
    UAPIV1::NetworkMagnitudeFromAmplitudesResponse::StationMagnitudeAndResidual
       result;
    result.set_residual(getValue()); // Throws
    const auto &amplitudePair
        = pImpl->mStationMagnitude.getAmplitudesReference();
    const Corrections::StationIdentifier stationIdentifier
    {
        amplitudePair.first.getIdentifier()
    };
    *result.mutable_identifier()
        = stationIdentifier.toMessage<UAPIV1::StationIdentifier> ();
    *result.mutable_station_magnitude()
        = pImpl->mStationMagnitude.toMessage <UAPIV1::StationMagnitude> ();
    return result;    
}
