#ifndef ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_STATION_MAGNITUDE_CALCULATOR_HPP
#define ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_STATION_MAGNITUDE_CALCULATOR_HPP
#include <memory>
#include <utility>
namespace ULocalMagnitudeService::Magnitude
{
 class Observation;
 class StationMagnitude;
}
namespace ULocalMagnitudeService::Corrections
{
 class Station;
 class Distance;
}

namespace ULocalMagnitudeService::Magnitude
{

/// @class StationMagnitudeCalculator stationMagnitudeCalculator.hpp
/// @brief Computes the station magnitudes the UUSS way - that is:
///           StationMagnitude = log10(AverageAmplitude/2) + C_d + C_s
///        where C_d is the distance correction and C_s the station correction.
/// @copyright Ben Baker (University of Utah) distributed under the 
///            MIT NO AI license.
class StationMagnitudeCalculator
{
public:
    /// @brief Constructor.
    /// @param[in] station   The station correction.
    /// @param[in] distance  The distance correction table.
    /// @throws std::invalid_argument if the station correction or the
    ///         distance corrections are not initialized.
    StationMagnitudeCalculator(const Corrections::Station &station,
                               const Corrections::Distance &distance);
    /// @brief Copy constructor.
    StationMagnitudeCalculator(const StationMagnitudeCalculator &calculator);
    /// @brief Move constructor.
    StationMagnitudeCalculator(StationMagnitudeCalculator &&calculator) noexcept;

    /// @result True indicates the class is initialized.
    [[nodiscard]] bool isInitialized() const noexcept;

    /// @brief Computes the station magnitude at the station given the
    ///        observation.
    /// @param[in] observation  The observed amplitudes and requisite
    ///                         source information for computing a
    ///                         station magnitude.
    /// @result The station magnitude along with the station and distance
    ///         corrections that were applied.
    /// @throws std::invalid_argument if the amplitudes are not set,
    ///         the epicentral distance is not set, and, if this is using
    ///         a depth correction, the source depth is not set.
    /// @throws std::runtime_error if \c isInitialized() is false.
    [[nodiscard]] StationMagnitude operator()(const Observation &observation) const;
 
    /// @brief Function to extract the distance correction.
    /// @param[in] epicentralDistance  The source-receiver epicentral distance
    ///                                in meters.
    /// @param[in] eventDepth          The event depth in meters.  This is
    ///                                only used when the distance corrections
    ///                                are based on hypocentral distance.
    /// @result The distance correction in magnitude units.
    /// @throws std::runtime_error if \c isInitialized() is false.
    [[nodiscard]] double getDistanceCorrection(double epicentralDistance,
                                               double eventDepth) const;
    /// @brief Function to extract the station correction.
    /// @result The station correction in magnitude units. 
    /// @throws std::runtime_error if \c isInitialized() is false.
    [[nodiscard]] double getStationCorrection() const;

    /// @brief Destructor.
    ~StationMagnitudeCalculator();
    /// @brief Copy assignment.
    StationMagnitudeCalculator& operator=(const StationMagnitudeCalculator &calculator);
    /// @brief Move assignment.
    StationMagnitudeCalculator& operator=(StationMagnitudeCalculator &&calculator) noexcept;
private:
    class StationMagnitudeCalculatorImpl;
    std::unique_ptr<StationMagnitudeCalculatorImpl> pImpl;
};

}
#endif
