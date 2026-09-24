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
#include "uLocalMagnitude/corrections/distanceOptions.hpp"

#define MAX_DISTANCE_METERS 21000000

namespace
{

/// Straight from Chuck Richter
const std::vector<std::pair<double, double>> utahCorrections
{
    {      0.0,   1.4}, {   5000.0,   1.4}, {  10000.0,   1.5}, {  15000.0,   1.6},
    {  20000.0,   1.7}, {  25000.0,   1.9}, {  30000.0,   2.1}, {  35000.0,   2.3},
    {  40000.0,   2.4}, {  45000.0,   2.5}, {  50000.0,   2.6}, {  55000.0,   2.7},
    {  60000.0,   2.8}, {  65000.0,   2.8}, {  70000.0,   2.8}, {  80000.0,   2.9},
    {  85000.0,   2.9}, {  90000.0,   3.0}, {  95000.0,   3.0}, { 100000.0,   3.0},
    { 110000.0,   3.1}, { 120000.0,   3.1}, { 130000.0,   3.2}, { 140000.0,   3.2},
    { 150000.0,   3.3}, { 160000.0,   3.3}, { 170000.0,   3.4}, { 180000.0,   3.4},
    { 190000.0,   3.5}, { 200000.0,   3.5}, { 210000.0,   3.6}, { 220000.0,  3.65},
    { 230000.0,   3.7}, { 240000.0,   3.7}, { 250000.0,   3.8}, { 260000.0,   3.8},
    { 270000.0,   3.9}, { 280000.0,   3.9}, { 290000.0,   4.0}, { 300000.0,   4.0},
    { 310000.0,   4.1}, { 320000.0,   4.1}, { 330000.0,   4.2}, { 340000.0,   4.2},
    { 350000.0,   4.3}, { 360000.0,   4.3}, { 370000.0,   4.3}, { 380000.0,   4.4},
    { 390000.0,   4.4}, { 400000.0,   4.5}, { 410000.0,   4.5}, { 420000.0,   4.5},
    { 430000.0,   4.6}, { 440000.0,   4.6}, { 450000.0,   4.6}, { 460000.0,   4.6},
    { 470000.0,   4.7}, { 480000.0,   4.7}, { 490000.0,   4.7}, { 500000.0,   4.7},
    { 510000.0,   4.8}, { 520000.0,   4.8}, { 530000.0,   4.8}, { 540000.0,   4.8},
    { 550000.0,   4.8}, { 560000.0,   4.9}, { 570000.0,   4.9}, { 580000.0,   4.9},
    { 590000.0,   4.9}, { 600000.0,   4.9}
};

/// The Yellowstone local magnitude distance corrections.  These are from
/// Holt et al., Recalibration of the Local Magnitude Scale for Earthquakes
/// in the Yellowstone Volcanic Region (2021).
const std::vector<std::pair<double, double>> yellowstoneCorrections
{
    {   3000.0,  0.64}, {   6000.0,  0.72}, {   9000.0,  0.87}, {  12000.0,  1.09},
    {  15000.0,  1.33}, {  18000.0,  1.55}, {  21000.0,  1.75}, {  25000.0,  1.94},
    {  30000.0,  2.11}, {  35000.0,  2.26}, {  40000.0,   2.4}, {  45000.0,  2.55},
    {  50000.0,  2.69}, {  55000.0,  2.82}, {  60000.0,  2.94}, {  65000.0,  3.04},
    {  70000.0,  3.11}, {  75000.0,  3.15}, {  80000.0,  3.17}, {  85000.0,  3.16},
    {  90000.0,  3.15}, {  95000.0,  3.12}, { 100000.0,  3.09}, { 105000.0,  3.07},
    { 110000.0,  3.06}, { 115000.0,  3.07}, { 120000.0,   3.1}, { 125000.0,  3.15},
    { 130000.0,  3.22}, { 135000.0,  3.29}, { 140000.0,  3.37}, { 145000.0,  3.44},
    { 150000.0,   3.5}, { 155000.0,  3.56}, { 160000.0,   3.6}, { 165000.0,  3.62},
    { 170000.0,  3.65}, { 175000.0,  3.66}, { 180000.0,  3.67}
};

}

using namespace ULocalMagnitude::Corrections;

class DistanceOptions::DistanceOptionsImpl
{
public:
    std::vector<std::pair<double, double>> mCorrections;
    DistanceOptions::Interpolation mInterpolation
    {
        DistanceOptions::Interpolation::Nearest
    };
    bool mHaveCorrections{false};
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

/// Correction strategy
void DistanceOptions::setInterpolation(
    const Interpolation interpolation) noexcept
{
    pImpl->mInterpolation = interpolation;
} 

DistanceOptions::Interpolation
DistanceOptions::getInterpolation() const noexcept
{
    return pImpl->mInterpolation;
}

/// Corrections table
/*
void DistanceOptions::setUtahCorrections()
{
   setCorrections(utahCorrections);
}

void DistanceOptions::setYellowstoneCorrections()
{
   setCorrections(yellowstoneCorrections);
}
*/

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

DistanceOptions ULocalMagnitude::Corrections::fromInitializationFile(
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
        = propertyTree.get<std::string> (section + "interpolation");
    interpolation = boost::algorithm::to_lower_copy(interpolation);
    if (interpolation == "linear")
    {
        options.setInterpolation(DistanceOptions::Interpolation::Linear);
    }
    else if (interpolation == "nearest")
    {
        options.setInterpolation(DistanceOptions::Interpolation::Nearest);
    }
    else
    {
        throw std::invalid_argument("interpolation type "
                                  + interpolation 
                                  + " must be nearest or linear");
    }
    // Fast path to load the table 
    if (propertyTree.get<bool> (section + "useUtahCorrections", false))
    {
        options.setCorrections(utahCorrections);
    }
    else if (propertyTree.get<bool> (section
                                   + "useYellowstoneCorrections", false))
    {
        options.setCorrections(yellowstoneCorrections);
    }
    else
    {
        // Parse this one thing at a time
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
    }
    return options;
}
