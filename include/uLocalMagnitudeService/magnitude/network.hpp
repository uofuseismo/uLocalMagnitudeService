#ifndef ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_NETWORK_HPP
#define ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_NETWORK_HPP
#include <memory>
#include <optional>
#include <vector>

namespace ULocalMagnitudeService::Corrections
{
 class Distance;
 class StationIdentifier;
}
namespace ULocalMagnitudeService::Magnitude
{
 class NetworkOptions;
 class Observation;
 class Residual;
}

namespace ULocalMagnitudeService::Magnitude
{
/// @class Network network.hpp
/// @brief Computes a network-based magnitude - loosely speaking an average of
///        station magnitudes.
/// @copyright Ben Baker (University of Utah) distributed under the
///            MIT NO AI license.
class Network
{
public:
    /// @brief Constructs the network magnitude.
    /// @throws std::invalid_argument if the station or distance corrections
    ///         are not set.
    explicit Network(const NetworkOptions &options);
    /// @brief Copy constructor.
    Network(const Network &network);
    /// @brief Move constructor.
    Network(Network &&network) noexcept; 

    /// @result True indicates the class is initialized.
    [[nodiscard]] bool isInitialized() const noexcept;   

    /// @result The distance corrections.
    /// @throws std::runtime_error if \c isInitialized() is false.
    [[nodiscard]] Corrections::Distance getDistanceCorrections() const;

    /// @result The station correction for the provided station.
    /// @throws std::runtime_error if \c isInitialized() is false.
    [[nodiscard]] std::optional<double> getStationCorrection(const Corrections::StationIdentifier &identifier) const;

    //void compute(const std::vector<Amplitude> &amplitudes);   
    //[[nodiscard]] Summary operator()(const std::vector<Observation> &observations) const;

    /// @brief Destructor.
    ~Network();
    /// @brief Move assignment.
    Network& operator=(Network &&network) noexcept;
    /// @brief Copy assignment.
    //Network& operator=(const Network &network);

    Network() = delete;
private:
    class NetworkImpl;
    std::unique_ptr<NetworkImpl> pImpl;
};
}
#endif
