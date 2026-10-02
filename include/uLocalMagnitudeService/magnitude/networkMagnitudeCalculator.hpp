#ifndef ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_NETWORK_MAGNITUDE_CALCULATOR_HPP
#define ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_NETWORK_MAGNITUDE_CALCULATOR_HPP
#include <memory>
#include <optional>
#include <vector>
#include <spdlog/spdlog.h>
namespace ULocalMagnitudeService::Corrections
{
 class Distance;
 class StationIdentifier;
}
namespace ULocalMagnitudeService::Magnitude
{
 class NetworkMagnitude;
 class NetworkMagnitudeCalculatorOptions;
 class Observation;
}

namespace ULocalMagnitudeService::Magnitude
{
/// @class NetworkMagnitudeCalculator networkMagnitudeCalculator.hpp
/// @brief Computes a network-based magnitude - loosely speaking an average of
///        station magnitudes.
/// @copyright Ben Baker (University of Utah) distributed under the
///            MIT NO AI license.
class NetworkMagnitudeCalculator
{
public:
    /// @brief Constructs the network magnitude calculator.
    /// @param[in] options  The network magnitude options.
    /// @param[in] logger   The logging utility.
    /// @throws std::invalid_argument if the station or distance corrections
    ///         are not set.
    NetworkMagnitudeCalculator(const NetworkMagnitudeCalculatorOptions &options,
                               std::shared_ptr<spdlog::logger> logger);
    /// @brief Copy constructor.
    NetworkMagnitudeCalculator(const NetworkMagnitudeCalculator &calculator);
    /// @brief Move constructor.
    NetworkMagnitudeCalculator(NetworkMagnitudeCalculator &&calculator) noexcept;

    /// @result True indicates the class is initialized.
    [[nodiscard]] bool isInitialized() const noexcept;   

    /// @result The distance corrections.
    /// @throws std::runtime_error if \c isInitialized() is false.
    [[nodiscard]] Corrections::Distance getDistanceCorrections() const;

    /// @result The station correction for the provided station.
    /// @throws std::runtime_error if \c isInitialized() is false.
    [[nodiscard]] std::optional<double> getStationCorrection(const Corrections::StationIdentifier &identifier) const;

    /// @brief Computes the network magnitude from the given observations.
    [[nodiscard]] NetworkMagnitude operator()(const std::vector<Observation> &observations) const;

    /// @brief Destructor.
    ~NetworkMagnitudeCalculator();
    /// @brief Copy assignment.
    NetworkMagnitudeCalculator& operator=(const NetworkMagnitudeCalculator &calculator);
    /// @brief Move assignment.
    NetworkMagnitudeCalculator& operator=(NetworkMagnitudeCalculator &&calculator) noexcept;

    NetworkMagnitudeCalculator() = delete;
private:
    class NetworkMagnitudeCalculatorImpl;
    std::unique_ptr<NetworkMagnitudeCalculatorImpl> pImpl;
};
}
#endif
