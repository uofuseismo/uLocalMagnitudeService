#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include "uLocalMagnitude/corrections/stationsSet.hpp"
#include "uLocalMagnitude/corrections/station.hpp"

using namespace ULocalMagnitude::Corrections;

class StationsSet::StationsSetImpl
{
public:
    std::map<std::string, Station> mCorrections;
};

/// Constructor
StationsSet::StationsSet() :
    pImpl(std::make_unique<StationsSetImpl> ())
{
}

/// Copy constructor
StationsSet::StationsSet(const StationsSet &set)
{
    *this = set;
}

/// Move constructor
StationsSet::StationsSet(StationsSet &&set) noexcept
{
    *this = std::move(set);
}

/// Copy assignent
StationsSet& StationsSet::operator=(const StationsSet &set)
{
    if (&set == this){return *this;}
    pImpl = std::make_unique<StationsSetImpl> (*set.pImpl);
    return *this;
}

/// Move assignent
StationsSet& StationsSet::operator=(StationsSet &&set) noexcept
{
    if (&set == this){return *this;}
    pImpl = std::move(set.pImpl);
    return *this;
}

/// Empty?
bool StationsSet::empty() const noexcept
{
    return pImpl->mCorrections.empty();
}

/// Destructor
StationsSet::~StationsSet() = default;

/// Insert
bool StationsSet::insert(const Station &correction, const bool overwrite)
{
    if (!correction.isInitialized())
    {
        throw std::invalid_argument("Correction not initialized");
    }
    auto name = correction.getName();
    if (pImpl->mCorrections.contains(name))
    {
        if (overwrite)
        {
            pImpl->mCorrections.insert_or_assign(name, correction);
            return true;
        }
        return false;
    }
    else
    {
        auto [it, inserted] 
           = pImpl->mCorrections.insert_or_assign(name, correction);
        return inserted;
    }
    return false;
}

/// Corrections
std::map<std::string, Station> StationsSet::getCorrections() const
{
    return pImpl->mCorrections;
}

/// Get the correction
std::optional<Station> 
StationsSet::getCorrection(const std::string &identifier) const
{
    if (identifier.empty()){return std::nullopt;}
    auto it = pImpl->mCorrections.find(identifier);
    if (it != pImpl->mCorrections.end())
    {
        return std::make_optional<Station> (it->second);
    }
    return std::nullopt;
}
