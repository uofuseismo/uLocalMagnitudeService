#ifndef ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_STATION_LOCATION_HPP
#define ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_STATION_LOCATION_HPP
#include <chrono>
#include <memory>
namespace ULocalMagnitudeService::Corrections
{
 class StationIdentifier;
}
namespace ULocalMagnitudeService::Corrections
{
/// @class StationLocation stationLocation.hpp
/// @brief Defines a station location in the WGS84 system.
/// @copyright Ben Baker (University of Utah) distributed under the
///            MIT NO AI license.
class StationLocation
{
public:
    /// @brief Constructor.
    StationLocation();
    /// @brief Copy constructor.
    StationLocation(const StationLocation &location);
    /// @brief Move constructor.
    StationLocation(StationLocation &&location) noexcept;

    /// @brief Sets the latitude.
    /// @param[in] latitude   The latitude in degrees.
    /// @throws std::invalid_argument if this is not in the range [-90, 90].
    void setLatitude(double latitude);
    /// @result The latitude of the station.
    /// @throws std::runtime_error if \c hasLatitude() is false.
    [[nodiscard]] double getLatitude() const;
    /// @result True indicates the latitude was set.
    [[nodiscard]] bool hasLatitude() const noexcept;

    /// @brief Sets the longitude.
    /// @param[in] longitude   The longitude in degrees.
    void setLongitude(double latitude) noexcept;
    /// @result The longitude of the station.
    /// @throws std::runtime_error if \c hasLongitude() is false.
    [[nodiscard]] double getLongitude() const;
    /// @result True indicates the longitude was set.
    [[nodiscard]] bool hasLongitude() const noexcept;

    /// @brief Sets the station elevation.
    /// @param[in] elevation   The station elevation in meters.
    ///                        This increases positive up from sea-level.
    /// @throws std::invalid_argument if the elevation is not in the
    ///         range [-10000, 8600].
    void setElevation(double elevation);
    /// @result The station elevation.
    /// @throws std::runtime_error if \c hasElevation() is false.
    [[nodiscard]] double getElevation() const;
    /// @result True indicates the elevation was set.
    [[nodiscard]] bool hasElevation() const noexcept;

    /// @brief Sets the station identifier.
    /// @param[in] identifier   The station identifier.
    /// @throws std::invalid_argument if \c hasNetwork() or \c hasStation()
    ///         is false.
    void setIdentifier(const StationIdentifier &identifier);
    /// @result The station identifier.
    /// @throws std::runtime_error if \c hasIdentifier() is false.
    [[nodiscard]] StationIdentifier getIdentifier() const;
    /// @result True indicates that the station identifier is set.
    [[nodiscard]] bool hasIdentifier() const noexcept;

    /// @brief Destructor.
    ~StationLocation();
    /// @brief Copy assignment.
    StationLocation& operator=(const StationLocation &location);
    /// @brief Move assignment.
    StationLocation& operator=(StationLocation &&location) noexcept;
private:
    class StationLocationImpl;
    std::unique_ptr<StationLocationImpl> pImpl;
};
}
#endif
