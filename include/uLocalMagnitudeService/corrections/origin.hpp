#ifndef ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_ORIGIN_HPP
#define ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_ORIGIN_HPP
#include <chrono>
#include <memory>
namespace ULocalMagnitudeService::Corrections
{
/// @class Origin origin.hpp
/// @brief Defines the where in of an event in a WGS84 system as well as
///        the UTC when in since the epoch (January 1970).
/// @copyright Ben Baker (University of Utah) distributed under the
///            MIT NO AI license.
class Origin
{
public:
    /// @brief Constructor.
    Origin();
    /// @brief Copy constructor.
    Origin(const Origin &origin);
    /// @brief Move constructor.
    Origin(Origin &&origin) noexcept;

    /// @brief Sets the latitude.
    /// @param[in] latitude   The latitude in degrees.
    /// @throws std::invalid_argument if this is not in the range [-90, 90] or
    ///         the latitude is not finite.
    void setLatitude(double latitude);
    /// @result The latitude of the event.
    /// @throws std::runtime_error if \c hasLatitude() is false.
    [[nodiscard]] double getLatitude() const;
    /// @result True indicates the latitude was set.
    [[nodiscard]] bool hasLatitude() const noexcept;

    /// @brief Sets the longitude.
    /// @param[in] longitude   The longitude in degrees.
    /// @throws std::invalid_argument if the longitude is not finite.
    void setLongitude(double longitude);
    /// @result The longitude of the event.
    /// @throws std::runtime_error if \c hasLongitude() is false.
    [[nodiscard]] double getLongitude() const;
    /// @result True indicates the longitude was set.
    [[nodiscard]] bool hasLongitude() const noexcept;

    /// @brief Sets the event depth.
    /// @param[in] depth   The event depth in meters.
    ///                    This increases positive down.
    /// @throws std::invalid_argument if the event depth is not in the range
    ///         [-8600, 900000] or the depth is not finite.
    void setDepth(double depth);
    /// @result The event depth.
    /// @throws std::runtime_error if \c hasDepth() is false.
    [[nodiscard]] double getDepth() const;
    /// @result True indicates the depth was set.
    [[nodiscard]] bool hasDepth() const noexcept;

    /// @brief Sets the origin time.
    /// @param[in] originTime   The origin time in UTC.
    void setTime(const std::chrono::nanoseconds &originTime) noexcept;
    /// @brief The event origin time.
    [[nodiscard]] std::chrono::nanoseconds getTime() const;
    /// @result True indicates that the origin time was set.
    [[nodiscard]] bool hasTime() const noexcept;

    /// @brief Destructor.
    ~Origin();
    /// @brief Copy assignment.
    Origin& operator=(const Origin &origin);
    /// @brief Move assignment.
    Origin& operator=(Origin &&origin) noexcept;
private:
    class OriginImpl;
    std::unique_ptr<OriginImpl> pImpl;
};
}
#endif

