#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>
#include "uLocalMagnitudeService/magnitude/networkMagnitude.hpp"
#include "uLocalMagnitudeService/magnitude/networkMagnitudeCalculator.hpp"
#include "uLocalMagnitudeService/magnitude/residual.hpp"
#include "uLocalMagnitudeServiceAPI/magnitude/v1/network_magnitude_from_amplitudes_response.pb.h"

using namespace ULocalMagnitudeService::Magnitude;

class NetworkMagnitude::NetworkMagnitudeImpl
{
public:
    std::vector
    <
        std::pair
        <
            Residual,
            int
        >
    > mResiduals;
    std::vector
    <
        std::pair
        <
             NetworkMagnitudeCalculator::StationCalculatorErrorCode,
             int
        >
    > mRejectedObservations; 
    double mValue{0};
    bool mHasValue{false};
    bool mHasResiduals{false};
    bool mHasRejectedObservations{false};
};

/// Constructor
NetworkMagnitude::NetworkMagnitude() :
    pImpl(std::make_unique<NetworkMagnitudeImpl> ())
{
}

/// Copy constructor
NetworkMagnitude::NetworkMagnitude(const NetworkMagnitude &magnitude)
{
    *this = magnitude;
}

/// Move constructor
NetworkMagnitude::NetworkMagnitude(NetworkMagnitude &&magnitude) noexcept
{
    *this = std::move(magnitude);
}

/// Copy assignment
NetworkMagnitude& NetworkMagnitude::operator=(const NetworkMagnitude &magnitude)
{
    if (&magnitude == this){return *this;}
    pImpl = std::make_unique<NetworkMagnitudeImpl> (*magnitude.pImpl);
    return *this;
}

/// Move assignment
NetworkMagnitude& 
NetworkMagnitude::operator=(NetworkMagnitude &&magnitude) noexcept
{
    if (&magnitude == this){return *this;}
    pImpl = std::move(magnitude.pImpl);
    return *this;
}

/// Destructor
NetworkMagnitude::~NetworkMagnitude() = default;

/// Value
void NetworkMagnitude::setValue(const double value)
{
    if (value <-10 || value > 10)
    {
        throw std::invalid_argument("Magnitude must be in range [-10, 10]");
    }
    pImpl->mValue = value;
    pImpl->mHasValue = true;
}

double NetworkMagnitude::getValue() const
{
    if (!hasValue())
    {
        throw std::runtime_error("Value not set");
    }
    return pImpl->mValue;
}

bool NetworkMagnitude::hasValue() const noexcept
{
    return pImpl->mHasValue;
} 

/// Sets the observations
void NetworkMagnitude::setResiduals(
    std::vector<std::pair<Residual, int>> &&residuals)
{
    if (residuals.empty())
    {
        throw std::invalid_argument("No residuals");
    }
    std::vector<std::pair<Residual, int>> workspace;
    workspace.reserve(residuals.size());
    for (auto &element : residuals)
    {
        if (!element.first.hasValue())
        {
            throw std::invalid_argument("Residual not set");
        }
        if (!element.first.hasStationMagnitude())
        {
            throw std::invalid_argument(
                "Station magnitude not set on residual");
        }
        if (element.second < 1)
        {
            throw std::invalid_argument("Observation number must be positive"); 
        }
        workspace.push_back(std::move(element));
    }
    pImpl->mResiduals = std::move(workspace);
    pImpl->mHasResiduals = true;
}

void NetworkMagnitude::setResiduals(
    const std::vector<std::pair<Residual, int>> &residuals)
{
    std::vector<std::pair<Residual, int>> copy{residuals};
    setResiduals(std::move(copy));
}

std::vector<std::pair<Residual, int>>
NetworkMagnitude::getResiduals() const
{
    if (!hasResiduals())
    {
        throw std::runtime_error("Residuals not set");
    }
    return pImpl->mResiduals;
}

const std::vector<std::pair<Residual, int>>
&NetworkMagnitude::getResidualsReference() const
{
    if (!hasResiduals())
    {   
        throw std::runtime_error("Residuals not set");
    }   
    return *&pImpl->mResiduals;
}


bool NetworkMagnitude::hasResiduals() const noexcept
{
    return pImpl->mHasResiduals;
}

/// Rejected observations
void NetworkMagnitude::setRejectedObservations(
    const std::vector
    <
        std::pair<NetworkMagnitudeCalculator::StationCalculatorErrorCode, int>
    > &rejectedObservations)
{
    std::vector
    <
        std::pair<NetworkMagnitudeCalculator::StationCalculatorErrorCode, int>
    > workspace;
    for (const auto &element : rejectedObservations)
    {
        const auto [reason, observationNumber] = element;
        if (observationNumber < 1)
        {
            throw std::invalid_argument("Observation number must be positive"); 
        }   
        workspace.push_back(element);
    }
    pImpl->mRejectedObservations = std::move(workspace);
    pImpl->mHasRejectedObservations = true;
}

std::vector
<
    std::pair<NetworkMagnitudeCalculator::StationCalculatorErrorCode, int>
>
NetworkMagnitude::getRejectedObservations() const
{
    if (!hasRejectedObservations())
    {
        throw std::runtime_error("Rejected observations not set");
    }
    return pImpl->mRejectedObservations;
}

bool NetworkMagnitude::hasRejectedObservations() const noexcept
{
    return pImpl->mHasRejectedObservations;
}

template<>
std::vector
<
    ULocalMagnitudeServiceAPI::Magnitude::V1
       ::NetworkMagnitudeFromAmplitudesResponse::RejectedObservation
>
NetworkMagnitude::toRejectedObservationsMessage() const
{
    if (!hasRejectedObservations())
    {
        throw std::runtime_error("Rejected observations not set");
    }
    using namespace ULocalMagnitudeServiceAPI::Magnitude::V1;
    std::vector
    <
        NetworkMagnitudeFromAmplitudesResponse::RejectedObservation
    > result;
    result.reserve(pImpl->mRejectedObservations.size());
    for (const auto &item : pImpl->mRejectedObservations)
    {
        NetworkMagnitudeFromAmplitudesResponse::RejectedObservation
            reject;
        auto reason
            = NetworkMagnitudeFromAmplitudesResponse::Reason
              ::NetworkMagnitudeFromAmplitudesResponse_Reason_REASON_UNSPECIFIED;
        if (item.first ==
            NetworkMagnitudeCalculator::StationCalculatorErrorCode
               ::StationTooClose)
        {
            reason
                = NetworkMagnitudeFromAmplitudesResponse::Reason
                  ::NetworkMagnitudeFromAmplitudesResponse_Reason_REASON_TOO_CLOSE;
        }
        else if (item.first ==
                 NetworkMagnitudeCalculator::StationCalculatorErrorCode
                    ::StationTooFar)
        {
            reason
                = NetworkMagnitudeFromAmplitudesResponse::Reason
                  ::NetworkMagnitudeFromAmplitudesResponse_Reason_REASON_TOO_FAR;
        }
        else if (item.first ==
                 NetworkMagnitudeCalculator::StationCalculatorErrorCode
                    ::ObservationHasNoCorrection)
        {
            reason
                = NetworkMagnitudeFromAmplitudesResponse::Reason
                  ::NetworkMagnitudeFromAmplitudesResponse_Reason_REASON_NO_STATION_CORRECTION;
        }
        // (1) The API uses hypocenters and station locations so a negative
        //     distance is my fault.
        // (2) A negative depth would be flagged by a bad hypocenter.
        // (3) Missing a distance or depth is another algorithm failure
        //     since I should have set it.
        else if (item.first ==
                 NetworkMagnitudeCalculator::StationCalculatorErrorCode
                    ::NegativeDistance ||
                 item.first ==
                 NetworkMagnitudeCalculator::StationCalculatorErrorCode
                    ::InvalidDepth ||
                 item.first ==
                 NetworkMagnitudeCalculator::StationCalculatorErrorCode
                    ::ObservationMissingDistance ||
                 item.first ==
                 NetworkMagnitudeCalculator::StationCalculatorErrorCode
                    ::ObservationMissingDepth)
        {
            reason
               = NetworkMagnitudeFromAmplitudesResponse::Reason
                  ::NetworkMagnitudeFromAmplitudesResponse_Reason_REASON_ALGORITHM;
        }
        else if (item.first ==
                 NetworkMagnitudeCalculator::StationCalculatorErrorCode
                    ::ObservationMissingAmplitudes)
        {
            reason
                = NetworkMagnitudeFromAmplitudesResponse::Reason
                  ::NetworkMagnitudeFromAmplitudesResponse_Reason_REASON_INVALID_AMPLITUDES;
        } 
        else if (item.first ==
                 NetworkMagnitudeCalculator::StationCalculatorErrorCode
                     ::Uninitialized ||
                 item.first ==
                 NetworkMagnitudeCalculator::StationCalculatorErrorCode
                     ::Algorithm)
        {
            reason
                = NetworkMagnitudeFromAmplitudesResponse::Reason
                  ::NetworkMagnitudeFromAmplitudesResponse_Reason_REASON_ALGORITHM;
        }
        reject.set_reason(reason);
        reject.set_input_observation_number(item.second);
        result.push_back(std::move(reject));
    }
    return result;
}

