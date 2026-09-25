#ifndef ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_DISTANCE_HPP
#define ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_DISTANCE_HPP
#include <memory>
#include <utility>
#include <vector>
namespace ULocalMagnitudeService::Corrections
{
 class DistanceOptions;
}
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
    /// @result The distance correction to add to the station magnitude - i.e.,
    ///         the output units are magnitude units.
    /// @throws std::invalid_argument if the distance is not positive
    ///         or exceeds 21,000,000 m (~ half the circumference of earth). 
    [[nodiscard]] double operator()(double distanceInMeters) const;

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
