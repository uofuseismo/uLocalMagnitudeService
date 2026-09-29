#include <cstdint>
#include <filesystem>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/constants.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim_all.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include "uLocalMagnitudeService/corrections/stationsSet.hpp"
#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"
#include "uLocalMagnitudeService/corrections/station.hpp"
#include "uLocalMagnitudeService/corrections/stationOptions.hpp"

using namespace ULocalMagnitudeService::Corrections;

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

/// Load from initialization file
StationsSet StationsSet::fromInitializationFile(
    const std::filesystem::path &iniFile,
    const std::string &sectionIn)
{
    if (!std::filesystem::exists(iniFile))
    {   
        throw std::invalid_argument("Initialization file "
                                  + iniFile.string() + " does not exist");
    }   
    StationsSet stationsSet;
    // Make sure section ends with . so we can find stuff
    auto section = sectionIn;
    if (!section.empty() && section.back() != '.'){section.append(".");}

    // Parse the initialization file
    boost::property_tree::ptree propertyTree;
    boost::property_tree::ini_parser::read_ini(iniFile, propertyTree);

    // Parse the table one entry at a time
    // station_correction_1 = UU.CWU,  0.3
    // station_correction_2 = US.DUG, -0.2
    // .
    // .
    // . 
    for (int i = 1; i < std::numeric_limits<uint16_t>::max(); ++i)
    {   
        auto itemName
            = section + "station_correction_" + std::to_string(i);
        auto stationValueString
            = propertyTree.get_optional<std::string> (itemName);
        if (stationValueString)
        {
            // Remove leading/trailing whitespace and duplicate blank
            // spaces from string so that there's at most one blank.
            // This handles case where user wants to do something like
            // correction_n = UU   CWU     2.3
            auto stationValue = *stationValueString;
            boost::algorithm::trim_all(stationValue);
            // Now split on commas, spaces, or tabs.  Compressing tokens
            // handles "UU.CWU, 2.3" where a comma and blank are adjacent.
            std::vector<std::string> splitString;
            boost::algorithm::split(splitString,
                                    stationValue,
                                    boost::is_any_of(",. \t"),
                                    boost::algorithm::token_compress_on);
            if (splitString.size() != 2)
            {
                throw std::invalid_argument(itemName
                  + " invalid format; need station_correction_n = string.string, number");
            }
            StationIdentifier identifier;
            auto network = splitString.at(0);
            auto station = splitString.at(1);
            identifier.setNetwork(network);
            identifier.setStation(station);
            auto correction = std::stod(splitString.at(2));
            StationOptions stationOptions;
            stationOptions.setIdentifier(identifier);
            stationOptions.setCorrection(correction);
            const Station stationCorrection{stationOptions};
            
            constexpr bool overwrite{false};
            auto added = stationsSet.insert(stationCorrection, overwrite);
            if (!added)
            {
                throw std::invalid_argument("Failed to add correction for "
                                          + identifier.toString()
                                          + "; likely a duplicate");
            }
        }
    }

    return stationsSet;
}
