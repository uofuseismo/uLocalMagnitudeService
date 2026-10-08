#ifndef ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_STATION_LOCATION_HPP
#define ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_STATION_LOCATION_HPP
#include <chrono>
#include <memory>
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
    /// @brief Creates the station location from a protobuf definition.
    /// @throws std::invalid_argument if the latitude, longitude, or elevation 
    ///         is not set, not finite, or the latitude or elevation is out 
    ///         of the usable range.
    /// @note Supported types: ULocalMagnitudeServiceAPI::V1::Magnitude::StationLocation.
    ///       Any other type fails to link.
    template<typename U>
    explicit StationLocation(const U &location);

    /// @brief Sets the latitude.
    /// @param[in] latitude   The latitude in degrees.
    /// @throws std::invalid_argument if this is not in the range [-90, 90] or
    ///         the latitude is not finite.
    void setLatitude(double latitude);
    /// @result The latitude of the station.
    /// @throws std::runtime_error if \c hasLatitude() is false.
    [[nodiscard]] double getLatitude() const;
    /// @result True indicates the latitude was set.
    [[nodiscard]] bool hasLatitude() const noexcept;

    /// @brief Sets the longitude.
    /// @param[in] longitude   The longitude in degrees.
    /// @throws std::invalid_argument if the longitude is not finite.
    void setLongitude(double longitude);
    /// @result The longitude of the station.
    /// @throws std::runtime_error if \c hasLongitude() is false.
    [[nodiscard]] double getLongitude() const;
    /// @result True indicates the longitude was set.
    [[nodiscard]] bool hasLongitude() const noexcept;

    /// @brief Sets the station elevation.
    /// @param[in] elevation   The station elevation in meters.
    ///                        This increases positive up from sea-level.
    /// @throws std::invalid_argument if the elevation is not in the
    ///         range [-10000, 8600] or the elevation is not finite.
    void setElevation(double elevation);
    /// @result The station elevation.
    /// @throws std::runtime_error if \c hasElevation() is false.
    [[nodiscard]] double getElevation() const;
    /// @result True indicates the elevation was set.
    [[nodiscard]] bool hasElevation() const noexcept;

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
