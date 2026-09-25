#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/constants.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim_all.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include "uLocalMagnitudeService/corrections/distanceOptions.hpp"

#define MAX_DISTANCE_METERS 21000000

using namespace ULocalMagnitudeService::Corrections;

class DistanceOptions::DistanceOptionsImpl
{
public:
    std::vector<std::pair<double, double>> mCorrections;
    DistanceOptions::Interpolation mInterpolation
    {
        DistanceOptions::Interpolation::Nearest
    };
    DistanceOptions::Type mType
    {
        DistanceOptions::Type::Epicentral
    };
    bool mHaveCorrections{false};
    bool mHasInterpolation{false};
    bool mHasType{false};
};

/// Constructor
DistanceOptions::DistanceOptions() :
    pImpl(std::make_unique<DistanceOptionsImpl> ())
{
}

/// Copy constructor
DistanceOptions::DistanceOptions(const DistanceOptions &options)
{
    *this = options;
}

/// Move constructor
DistanceOptions::DistanceOptions(DistanceOptions &&options) noexcept
{
    *this = std::move(options);
}

/// Destructor
DistanceOptions::~DistanceOptions() = default;

/// Copy assignent
DistanceOptions& DistanceOptions::operator=(const DistanceOptions &options)
{
    if (&options == this){return *this;}
    pImpl = std::make_unique<DistanceOptionsImpl> (*options.pImpl);
    return *this;
}

/// Move assignent
DistanceOptions& DistanceOptions::operator=(DistanceOptions &&options) noexcept
{
    if (&options == this){return *this;}
    pImpl = std::move(options.pImpl);
    return *this;
}

/// Interpolation type
void DistanceOptions::setInterpolation(
    const Interpolation interpolation) noexcept
{
    pImpl->mInterpolation = interpolation;
    pImpl->mHasInterpolation = true;
} 

DistanceOptions::Interpolation
DistanceOptions::getInterpolation() const
{
    if (!hasInterpolation())
    {
        throw std::runtime_error("Interpolation type not set");
    }
    return pImpl->mInterpolation;
}

bool DistanceOptions::hasInterpolation() const noexcept
{
    return pImpl->mHasInterpolation;
}

/// Meaning of distance
void DistanceOptions::setType(const Type type) noexcept
{
    pImpl->mType = type;
    pImpl->mHasType = true;
} 

DistanceOptions::Type DistanceOptions::getType() const
{
    if (!hasType())
    {   
        throw std::runtime_error("Distance type not set");
    }   
    return pImpl->mType;
}

bool DistanceOptions::hasType() const noexcept
{
    return pImpl->mHasType;
}


/// Corrections table
void DistanceOptions::setCorrections(
    const std::vector<std::pair<double, double>> &corrections)
{
    if (corrections.empty())
    {
        throw std::invalid_argument("Corrections table is empty");
    }
    auto hasNegativeDistance
        =  std::ranges::any_of(corrections, [](const auto &item)
           {
                return item.first < 0;
           });
    if (hasNegativeDistance)
    {
        throw std::invalid_argument(
           "All corrections distances must be non-negative");
    }
    auto correctionsSorted = corrections;
    // Sort ascending (helps interpolation)
    std::sort(correctionsSorted.begin(),
              correctionsSorted.end(),
              [](const auto &lhs, const auto &rhs)
              {
                  return lhs.first < rhs.first;
              });
    for (int i = 0; i < static_cast<int> (correctionsSorted.size() - 1); ++i)
    {
        constexpr double tol{0.1}; // 10 cm is this same spot in practice
        if (std::abs(correctionsSorted[i].first
                   - correctionsSorted[i + 1].first) < tol)
        {
            throw std::invalid_argument(
                "Duplicate correction at distance "
              + std::to_string(correctionsSorted[i].first));
        }
    }
    pImpl->mCorrections = std::move(correctionsSorted);
    pImpl->mHaveCorrections = true;
}

std::vector<std::pair<double, double>> DistanceOptions::getCorrections() const
{
    if (!hasCorrections())
    { 
        throw std::runtime_error("Corrections not set");
    }
    return pImpl->mCorrections;
}

const std::vector<std::pair<double, double>> 
&DistanceOptions::getCorrectionsReference() const
{
    if (!hasCorrections())
    {
        throw std::runtime_error("Corrections not set");
    }
    return *&pImpl->mCorrections;
}
 
bool DistanceOptions::hasCorrections() const noexcept
{
    return pImpl->mHaveCorrections;
}

DistanceOptions ULocalMagnitudeService::Corrections::fromInitializationFile(
    const std::filesystem::path &iniFile,
    const std::string &sectionIn)
{
    if (!std::filesystem::exists(iniFile))
    {
        throw std::invalid_argument("Initialization file "
                                  + iniFile.string() + " does not exist");
    }
    DistanceOptions options;
    // Make sure section ends with . so we can find stuff
    auto section = sectionIn;
    if (!section.empty() && section.back() != '.'){section.append(".");}

    // Parse the initialization file
    boost::property_tree::ptree propertyTree;
    boost::property_tree::ini_parser::read_ini(iniFile, propertyTree);

    auto interpolation
        = propertyTree.get_optional<std::string> (section + "interpolation");
    if (!interpolation)
    {
        throw std::invalid_argument(section + "interpolation not set");
    }
    auto interpolationType
        = boost::algorithm::to_lower_copy(*interpolation);
    if (interpolationType == "linear")
    {
        options.setInterpolation(DistanceOptions::Interpolation::Linear);
    }
    else if (interpolationType == "nearest")
    {
        options.setInterpolation(DistanceOptions::Interpolation::Nearest);
    }
    else
    {
        throw std::invalid_argument("interpolation type "
                                  + interpolationType
                                  + " must be nearest or linear");
    }

    auto distanceType
        = propertyTree.get_optional<std::string> (section + "distanceType");
    if (!distanceType)
    {
        throw std::invalid_argument(section + "distanceType not set");
    }
    auto type = boost::algorithm::to_lower_copy(*distanceType);
    if (type == "epicentral")
    {
        options.setType(DistanceOptions::Type::Epicentral);
    }
    else if (type == "hypocentral")
    {
        options.setType(DistanceOptions::Type::Hypocentral);
    }
    else
    {
        throw std::invalid_argument("distance type "
                                  + type
                                  + " must be epicentral or hypocentral");
    }

    // Parse the table one entry at a time
    // distance_correction_1 = 0,    1.4
    // distance_correction_2 = 5000, 1.4
    // .
    // .
    // . 
    std::vector<std::pair<double, double>> corrections;
    corrections.reserve(256);
    for (int i = 1; i < std::numeric_limits<uint16_t>::max(); ++i)
    {
        auto itemName
            = section + "distance_correction_" + std::to_string(i);
        auto distanceValueString
            = propertyTree.get_optional<std::string> (itemName);
        if (distanceValueString)
        {
            // Remove leading/trailing whitespace and duplicate blank
            // spaces from string so that there's at most one blank.
            // This handles case where user wants to do something like
            // correction_n = 2000   2.3
            auto distanceValue = *distanceValueString;
            boost::algorithm::trim_all(distanceValue);
            // Now split on commas, spaces, or tabs.  Compressing tokens
            // handles "2000, 2.3" where a comma and blank are adjacent.
            std::vector<std::string> splitString;
            boost::algorithm::split(splitString,
                                    distanceValue,
                                    boost::is_any_of(", \t"),
                                    boost::algorithm::token_compress_on);
            if (splitString.size() != 2)
            {
                throw std::invalid_argument(itemName
                  + " invalid format; need distance_correction_n = number, number");
            }
            auto distance = std::stod(splitString.at(0));
            auto value = std::stod(splitString.at(1));
            corrections.push_back(std::pair {distance, value}); 
        }
        else
        {
            break;
        }
    }
    options.setCorrections(corrections);
    return options;
}
