#include <algorithm>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#ifndef NDEBUG
#include <cassert>
#endif
#include "uLocalMagnitudeService/corrections/distance.hpp"
#include "uLocalMagnitudeService/corrections/distanceOptions.hpp"

#define MAX_DISTANCE_METERS 21000000

using namespace ULocalMagnitudeService::Corrections;

namespace
{

double interpolateNearest(
    const double x,
    const std::vector<double> &xv, const std::vector<double> &yv)
{
    // Safely do nothing
    if (xv.empty()){return 0;}
    // Kinda stupid
    if (xv.size() == 1){return yv[0];} 
    // Get to it
    auto it = std::lower_bound(xv.begin(), xv.end(), x);
    if (it == xv.begin()){return yv[0];} // At or below first element
    if (it == xv.end()){return yv.back();} // At or above last element
    // Here, *(it - 1) < x <= *it; pick whichever is closer
    auto i = static_cast<int> (std::distance(xv.begin(), it));
    auto nearestNeighborIndex
        = (x - xv[i - 1] <= xv[i] - x) ? i - 1 : i;
    return yv[nearestNeighborIndex];
}

double interpolateLinearly(
    const double x,
    const std::vector<double> &xv, const std::vector<double> &yv)
{
    // Safely do nothing
    if (xv.empty()){return 0;}
    // Kinda stupid
    if (xv.size() == 1){return yv[0];} 
    // Handle extrema up front by saturating 
    if (x <= xv.front()){return yv[0];}
    if (x >= xv.back()){return yv.back();}
    // Go to work
    auto it = std::upper_bound(xv.begin(), xv.end(), x);
    if (it == xv.begin() || it == xv.end())
    {
        throw std::runtime_error("Algorithmic error in binary search");
    }
    auto i = std::max(0, static_cast<int> (std::distance(xv.begin(), it)) - 1);
    return yv[i] + (x - xv[i])*(yv[i + 1] - yv[i])/(xv[i + 1] - xv[i]);
}

}

class Distance::DistanceImpl
{
public:
    DistanceOptions mDistanceOptions;
    std::vector<double> mAbsissas;
    std::vector<double> mValues;
    bool mLinearInterpolation{false};
    bool mInitialized{false};
};

/// Constructor
Distance::Distance(const DistanceOptions &options) :
    pImpl(std::make_unique<DistanceImpl> ())
{
    if (!options.hasCorrections())
    {
        throw std::runtime_error("No corrections set");
    }
    pImpl->mDistanceOptions = options;
    const auto correctionsTable
         = pImpl->mDistanceOptions.getCorrectionsReference();
    pImpl->mAbsissas.reserve(correctionsTable.size());
    pImpl->mValues.reserve(correctionsTable.size());
    for (const auto &correction : correctionsTable)
    {   
        pImpl->mAbsissas.push_back(correction.first);
        pImpl->mValues.push_back(correction.second);
    }   
    pImpl->mInitialized = true;
}

/// Copy constructor
Distance::Distance(const Distance &distance)
{
    *this = distance;
}

/// Move constructor
Distance::Distance(Distance &&distance) noexcept
{
    *this = std::move(distance);
}

/// Copy assignment
Distance &Distance::operator=(const Distance &distance)
{
    if (&distance == this){return *this;}
    pImpl = std::make_unique<DistanceImpl> (*distance.pImpl);
    return *this;
}

/// Move assignment
Distance &Distance::operator=(Distance &&distance) noexcept
{
    if (&distance == this){return *this;}
    pImpl = std::move(distance.pImpl);
    return *this;
}

/// Destructor
Distance::~Distance() = default;

/// Distance corrections since it is in the API
std::vector<std::pair<double, double>> Distance::getCorrections() const
{
    if (!isInitialized())
    {
        throw std::runtime_error("Distance corrections not initialized");
    }
    return pImpl->mDistanceOptions.getCorrections();
}

/// Initialized?
bool Distance::isInitialized() const noexcept
{
    return pImpl->mInitialized;
}

/// Operator to get it done
double Distance::operator()(const double distance) const
{
    if (!isInitialized())
    {   
        throw std::runtime_error("Distance corrections not initialized");
    }   
    if (distance < 0)
    {   
        throw std::invalid_argument("Interpolation distance is negative");
    }   
    if (distance > MAX_DISTANCE_METERS)
    {   
        throw std::invalid_argument("Interpolation distance cannote exceed "
                                  + std::to_string(MAX_DISTANCE_METERS));
    }   
    auto isNearest 
       = (pImpl->mDistanceOptions.getInterpolation() ==
          DistanceOptions::Interpolation::Nearest) ? true : false;
    if (isNearest)
    {
        return ::interpolateNearest(
                   distance, pImpl->mAbsissas, pImpl->mValues);
    }
    else
    {
        return ::interpolateLinearly(
                   distance, pImpl->mAbsissas, pImpl->mValues);
    }
}

