#include <memory>
#include <stdexcept>
#include <utility>
#include "uLocalMagnitudeService/magnitude/networkMagnitudeCalculatorOptions.hpp"
#include "uLocalMagnitudeService/corrections/distance.hpp"
#include "uLocalMagnitudeService/corrections/stationsSet.hpp"

using namespace ULocalMagnitudeService;
using namespace ULocalMagnitudeService::Magnitude;

class NetworkMagnitudeCalculatorOptions::NetworkMagnitudeCalculatorOptionsImpl
{
public:
    NetworkMagnitudeCalculatorOptionsImpl() = default;
    NetworkMagnitudeCalculatorOptionsImpl(const NetworkMagnitudeCalculatorOptionsImpl &options)
    {
        if (options.mDistanceCorrections != nullptr)
        {
            mDistanceCorrections
                = std::make_unique<Corrections::Distance>
                  (*options.mDistanceCorrections);
        }
        mStationCorrections = options.mStationCorrections;
        mMinimumNumberOfStationMagnitudes
            = options.mMinimumNumberOfStationMagnitudes;
        mStrategy = options.mStrategy;;
        mHasDistanceCorrections = options.mHasDistanceCorrections;
        mHasStationCorrections = options.mHasStationCorrections;
    }

    std::unique_ptr<Corrections::Distance> mDistanceCorrections;
    Corrections::StationsSet mStationCorrections;
    int mMinimumNumberOfStationMagnitudes{2};
    Strategy mStrategy{NetworkMagnitudeCalculatorOptions::Strategy::Average};
    bool mHasDistanceCorrections{false};
    bool mHasStationCorrections{false};
};

/// Constructor
NetworkMagnitudeCalculatorOptions::NetworkMagnitudeCalculatorOptions() :
    pImpl(std::make_unique<NetworkMagnitudeCalculatorOptionsImpl> ())
{
}

/// Copy constructor
NetworkMagnitudeCalculatorOptions::NetworkMagnitudeCalculatorOptions(const NetworkMagnitudeCalculatorOptions &options)
{
    *this = options;
}

/// Move constructor
NetworkMagnitudeCalculatorOptions::NetworkMagnitudeCalculatorOptions(NetworkMagnitudeCalculatorOptions &&options) noexcept
{
    *this = std::move(options);
}

/// Copy assignment
NetworkMagnitudeCalculatorOptions &NetworkMagnitudeCalculatorOptions::operator=(const NetworkMagnitudeCalculatorOptions &options)
{
    if (&options == this){return *this;}
    pImpl = std::make_unique<NetworkMagnitudeCalculatorOptionsImpl> (*options.pImpl);
    return *this;
}

/// Move assignment
NetworkMagnitudeCalculatorOptions &NetworkMagnitudeCalculatorOptions::operator=(NetworkMagnitudeCalculatorOptions &&options) noexcept
{
    if (&options == this){return *this;}
    pImpl = std::move(options.pImpl);
    return *this;
}

/// Destructor
NetworkMagnitudeCalculatorOptions::~NetworkMagnitudeCalculatorOptions() = default;

/// Distance corrections
void NetworkMagnitudeCalculatorOptions::setDistanceCorrections(
    const Corrections::Distance &corrections)
{
    if (!corrections.isInitialized())
    {
        throw std::invalid_argument("Distance corrections not initialized");
    }   
    pImpl->mDistanceCorrections
         = std::make_unique<Corrections::Distance> (corrections);
    pImpl->mHasDistanceCorrections = true;
}

Corrections::Distance NetworkMagnitudeCalculatorOptions::getDistanceCorrections() const
{
    if (!hasDistanceCorrections())
    {   
        throw std::runtime_error("Distance corrections not set");
    }   
    return *pImpl->mDistanceCorrections;
}

const Corrections::Distance 
&NetworkMagnitudeCalculatorOptions::getDistanceCorrectionsReference() const
{
    if (!hasDistanceCorrections())
    {   
        throw std::runtime_error("Distance corrections not set");
    }   
    return *&*pImpl->mDistanceCorrections;
}


bool NetworkMagnitudeCalculatorOptions::hasDistanceCorrections() const noexcept
{
    return pImpl->mHasDistanceCorrections;
}


/// Station corrections
void NetworkMagnitudeCalculatorOptions::setStationCorrections(
    const Corrections::StationsSet &corrections)
{
    if (corrections.empty())
    {
        throw std::invalid_argument("No station corrections");
    }
    pImpl->mStationCorrections = corrections;
    pImpl->mHasStationCorrections = true;
}

Corrections::StationsSet NetworkMagnitudeCalculatorOptions::getStationCorrections() const
{
    if (!hasStationCorrections())
    {
        throw std::runtime_error("Station corrections not set");
    }
    return pImpl->mStationCorrections;
}

bool NetworkMagnitudeCalculatorOptions::hasStationCorrections() const noexcept
{
    return pImpl->mHasStationCorrections;
}

/// Minimum number of observations
void NetworkMagnitudeCalculatorOptions::setMinimumNumberOfStationMagnitudes(const int minObs)
{
    if (minObs < 1)
    {
        throw std::invalid_argument(
            "Minimum number of stations magnitudes must be positive");
    }
    pImpl->mMinimumNumberOfStationMagnitudes = minObs;
}

int NetworkMagnitudeCalculatorOptions::getMinimumNumberOfStationMagnitudes() const noexcept
{
    return pImpl->mMinimumNumberOfStationMagnitudes;
}

/// Strategy
void NetworkMagnitudeCalculatorOptions::setStrategy(const Strategy strategy) noexcept
{
    pImpl->mStrategy = strategy;
}

NetworkMagnitudeCalculatorOptions::Strategy NetworkMagnitudeCalculatorOptions::getStrategy() const noexcept
{
    return pImpl->mStrategy;
}

/// Validate
void NetworkMagnitudeCalculatorOptions::validate() const
{
    if (!hasStationCorrections())
    {
        throw std::runtime_error("Station corrections not set");
    }
    if (!hasDistanceCorrections())
    {
        throw std::runtime_error("Distance corrections not set");
    }
}
