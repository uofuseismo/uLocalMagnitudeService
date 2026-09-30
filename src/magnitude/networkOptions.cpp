#include <memory>
#include <stdexcept>
#include <utility>
#include "uLocalMagnitudeService/magnitude/networkOptions.hpp"
#include "uLocalMagnitudeService/corrections/distance.hpp"
#include "uLocalMagnitudeService/corrections/stationsSet.hpp"

using namespace ULocalMagnitudeService;
using namespace ULocalMagnitudeService::Magnitude;

class NetworkOptions::NetworkOptionsImpl
{
public:
    NetworkOptionsImpl() = default;
    NetworkOptionsImpl(const NetworkOptionsImpl &options)
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
    Strategy mStrategy{NetworkOptions::Strategy::Average};
    bool mHasDistanceCorrections{false};
    bool mHasStationCorrections{false};
};

/// Constructor
NetworkOptions::NetworkOptions() :
    pImpl(std::make_unique<NetworkOptionsImpl> ())
{
}

/// Copy constructor
NetworkOptions::NetworkOptions(const NetworkOptions &options)
{
    *this = options;
}

/// Move constructor
NetworkOptions::NetworkOptions(NetworkOptions &&options) noexcept
{
    *this = std::move(options);
}

/// Copy assignment
NetworkOptions &NetworkOptions::operator=(const NetworkOptions &options)
{
    if (&options == this){return *this;}
    pImpl = std::make_unique<NetworkOptionsImpl> (*options.pImpl);
    return *this;
}

/// Move assignment
NetworkOptions &NetworkOptions::operator=(NetworkOptions &&options) noexcept
{
    if (&options == this){return *this;}
    pImpl = std::move(options.pImpl);
    return *this;
}

/// Destructor
NetworkOptions::~NetworkOptions() = default;

/// Distance corrections
void NetworkOptions::setDistanceCorrections(
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

Corrections::Distance NetworkOptions::getDistanceCorrections() const
{
    if (!hasDistanceCorrections())
    {   
        throw std::runtime_error("Distance corrections not set");
    }   
    return *pImpl->mDistanceCorrections;
}

const Corrections::Distance 
&NetworkOptions::getDistanceCorrectionsReference() const
{
    if (!hasDistanceCorrections())
    {   
        throw std::runtime_error("Distance corrections not set");
    }   
    return *&*pImpl->mDistanceCorrections;
}


bool NetworkOptions::hasDistanceCorrections() const noexcept
{
    return pImpl->mHasDistanceCorrections;
}


/// Station corrections
void NetworkOptions::setStationCorrections(
    const Corrections::StationsSet &corrections)
{
    if (corrections.empty())
    {
        throw std::invalid_argument("No station corrections");
    }
    pImpl->mStationCorrections = corrections;
    pImpl->mHasStationCorrections = true;
}

Corrections::StationsSet NetworkOptions::getStationCorrections() const
{
    if (!hasStationCorrections())
    {
        throw std::runtime_error("Station corrections not set");
    }
    return pImpl->mStationCorrections;
}

bool NetworkOptions::hasStationCorrections() const noexcept
{
    return pImpl->mHasStationCorrections;
}

/// Minimum number of observations
void NetworkOptions::setMinimumNumberOfStationMagnitudes(const int minObs)
{
    if (minObs < 1)
    {
        throw std::invalid_argument(
            "Minimum number of stations magnitudes must be positive");
    }
    pImpl->mMinimumNumberOfStationMagnitudes = minObs;
}

int NetworkOptions::getMinimumNumberOfStationMagnitudes() const noexcept
{
    return pImpl->mMinimumNumberOfStationMagnitudes;
}

/// Strategy
void NetworkOptions::setStrategy(const Strategy strategy) noexcept
{
    pImpl->mStrategy = strategy;
}

NetworkOptions::Strategy NetworkOptions::getStrategy() const noexcept
{
    return pImpl->mStrategy;
}

/// Validate
void NetworkOptions::validate() const
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
