#ifndef ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_HYPOCENTER_HPP
#define ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_HYPOCENTER_HPP
#include <memory>
namespace ULocalMagnitudeService::Corrections
{
/// @class Hypocenter hypocenter.hpp
/// @brief Defines the where in of an event in a WGS84 system.
/// @copyright Ben Baker (University of Utah) distributed under the
///            MIT NO AI license.
class Hypocenter
{
public:
    /// @brief Constructor.
    Hypocenter();
    /// @brief Copy constructor.
    Hypocenter(const Hypocenter &hypocenter);
    /// @brief Move constructor.
    Hypocenter(Hypocenter &&hypocenter) noexcept;
    /// @brief Creates the hypocenter from a protobuf definition.
    /// @throws std::invalid_argument if the latitude, longitude, or depth
    ///         is not set, not finite, or the latitude or depth is out 
    ///         of the usable range.
    /// @note Supported types: ULocalMagnitudeServiceAPI::V1::Magnitude::Hypocenter.
    ///       Any other type fails to link.
    template<typename U>
    explicit Hypocenter(const U &hypocenter);

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

    /// @brief Destructor.
    ~Hypocenter();
    /// @brief Copy assignment.
    Hypocenter& operator=(const Hypocenter &hypocenter);
    /// @brief Move assignment.
    Hypocenter& operator=(Hypocenter &&hypocenter) noexcept;
private:
    class HypocenterImpl;
    std::unique_ptr<HypocenterImpl> pImpl;
};
}
#endif

