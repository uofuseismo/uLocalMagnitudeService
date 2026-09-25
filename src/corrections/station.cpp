#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include "uLocalMagnitude/corrections/station.hpp"
#include "uLocalMagnitude/corrections/stationOptions.hpp"
#include "uLocalMagnitude/corrections/stationIdentifier.hpp"

using namespace ULocalMagnitude::Corrections;

class Station::StationImpl
{
public:
    StationOptions mOptions;
    double mCorrection{0};
    bool mInitialized{false};
};

/// Constructor
Station::Station(const StationOptions &options) :
    pImpl(std::make_unique<StationImpl> ())
{
    if (!options.hasIdentifier())
    {
        throw std::invalid_argument("Station identifier not set");
    }
    if (!options.hasCorrection())
    {
        throw std::invalid_argument("Correction not set");
    }
    pImpl->mOptions = options;
    pImpl->mCorrection = options.getCorrection();
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

/// Initialized
bool Station::isInitialized() const noexcept
{
    return pImpl->mInitialized;
}

/// Operator
double Station::operator()() const
{
    if (!isInitialized())
    {
        throw std::runtime_error("Station correction not initialized");
    }
    return pImpl->mCorrection;
}

/// Name
std::string Station::getName() const
{
    if (!isInitialized())
    {   
        throw std::runtime_error("Station correction not initialized");
    }   
    return pImpl->mOptions.getIdentifier().toString();
}
