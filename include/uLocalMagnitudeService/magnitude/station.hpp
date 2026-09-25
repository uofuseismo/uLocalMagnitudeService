#ifndef ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_STATION_HPP
#define ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_STATION_HPP
#include <memory>
#include <utility>
namespace ULocalMagnitudeService::Magnitude
{
 class Amplitude;
}
namespace ULocalMagnitudeService::Corrections
{
 class Station;
 class Distance;
}

namespace ULocalMagnitudeService::Magnitude
{

/// @class Station station.hpp
/// @brief Computes the station magnitudes the UUSS way - that is:
///           StationMagnitude = log10(AverageAmplitude/2) + C_d + C_s
///        where C_d is the distance correction and C_s the station correction.
/// @copyright Ben Baker (University of Utah) distributed under the 
///            MIT NO AI license.
class Station
{
public:
    /// @brief Constructor.
    /// @param[in] station   The station correction.
    /// @param[in] distance  The distance correction table.
    /// @throws std::invalid_argument if the station correction or the
    ///         distance corrections are not initialized.
    Station(const Corrections::Station &station,
            const Corrections::Distance &distance);
    /// @brief Copy constructor.
    Station(const Station &station);
    /// @brief Move constructor.
    Station(Station &&station) noexcept;

    /// @result True indicates the class is initialized.
    [[nodiscard]] bool isInitialized() const noexcept;

    /// @brief Computes the station magnitude at the station given the
    ///        amplitudes on the non-vertical channels.
    /// @param[in] amplitudePair       The observed amplitude on the
    ///                                non-vertical channels.
    /// @param[in] epicentralDistance  The source-receiver epicentral distance
    ///                                in meters.
    /// @param[in] eventDepth          The event depth in meters.  This is
    ///                                only used when the distance corrections
    ///                                are based on hypocentral distance.
    /// @throws std::invalid_argument if the stream identifiers are the same,
    ///         do not match the station identifier, the distance is
    ///         negative, or the depth is out of range.
    /// @throws std::runtime_error if \c isInitialized() is false.
    [[nodiscard]] double operator()(const std::pair<Amplitude, Amplitude> &amplitudePair,
                                    double epicentralDistance,
                                    double eventDepth) const;
 
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
    ~Station();
    /// @brief Copy constructor.
    Station& operator=(const Station &station);
    /// @brief Move constructor.
    Station& operator=(Station &&station) noexcept;
private:
    class StationImpl;
    std::unique_ptr<StationImpl> pImpl;
};

}
#endif
