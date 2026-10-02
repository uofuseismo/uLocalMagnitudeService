#ifndef ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_DISTANCE_HPP
#define ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_DISTANCE_HPP
#include <expected>
#include <memory>
#include <utility>
#include <vector>
#include <uLocalMagnitudeService/corrections/distanceOptions.hpp>

namespace ULocalMagnitudeService::Corrections
{
/// @class Distance distance.hpp
/// @brief Defines the distance correction which is added to the station's
///        local magnitude.
/// @copyright Ben Baker (University of Utah) distributed under the
///            MIT NO AI license.
class Distance
{
public:
    enum class ErrorCode
    {
        NegativeDistance,     /*!< The source receiver distance is negative. */
        StationTooFar,        /*!< The source receiver distance extends past the
                                   maximum distance in the correction table. */
        InvalidSourceDepth,   /*!< The source depth is being used to compute
                                   the hypocentral distance (when applicable)
                                   but it is less than -8600 m or greater than
                                   900000 m. */
        Uninitialized         /*!< The calculator is not initialized. */
    };
public:
    /// @brief Constructor.
    explicit Distance(const DistanceOptions &options);
    /// @brief Copy constructor.
    Distance(const Distance &distance);
    /// @brief Move constructor.
    Distance(Distance &&distance) noexcept;

    /// @result True indicates the class is initialized.
    [[nodiscard]] bool isInitialized() const noexcept;

    /// @brief Computes the corresponding distance correction.
    /// @param[in] distanceInMeters   The source-receiver distance in meters.
    ///                               The distance (hypocentral vs epicentral)
    ///                               is contextualized by \c getDistanceType().
    /// @result The distance correction to add to the station magnitude - i.e.,
    ///         the output units are magnitude units.
    [[nodiscard]] auto operator()(double distanceInMeters) const noexcept -> std::expected<double, ErrorCode>;
    /// @brief Computes the corresponding distance correction. 
    /// @param[in] epicentralDistance   The source-receiver epicentral
    ///                                 distance in meters
    /// @param[in] eventDepth  The event depth in meters.
    /// @note If the distance type is epicentral then the event depth is ignored. 
    [[nodiscard]] auto operator()(double epicentralDistance, double eventDepth) const noexcept -> std::expected<double, ErrorCode>;

    /// @result The distance type (context for the operator()).
    /// @throws std::runtime_error if \c isInitialized() is false.
    [[nodiscard]] DistanceOptions::Type getDistanceType() const;

    /// @result The maximum distance in meters in the distance table.
    /// @throws std::runtime_error if \c isInitialized() is false.
    [[nodiscard]] double getMaximumDistance() const; 

    /// @result The distance corrections table.
    [[nodiscard]] std::vector<std::pair<double, double>> getCorrections() const;

    /// @brief Destructor.
    ~Distance();
    /// @brief Copy assignment operator.
    Distance& operator=(const Distance &distance); 
    /// @brief Move assignment operator.
    Distance& operator=(Distance &&distance) noexcept;
private:
    class DistanceImpl;
    std::unique_ptr<DistanceImpl> pImpl;
};
}
#endif
