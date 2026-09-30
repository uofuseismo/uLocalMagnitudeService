#ifndef ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_NETWORK_OPTIONS_HPP
#define ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_NETWORK_OPTIONS_HPP
#include <memory>
namespace ULocalMagnitudeService::Corrections
{
 class Distance;
 class StationsSet;
}
namespace ULocalMagnitudeService::Magnitude
{
/// @class Network network.hpp
/// @brief Defines the options for computing a network magnitude.
/// @copyright Ben Baker (University of Utah) distributed under the
///            MIT NO AI license.
class NetworkOptions
{
public:
    enum class Strategy
    {
        Average  /*!< Computes a magnitude via a straight average. */
    };
public:
    /// @brief Constructor.
    NetworkOptions();
    /// @brief Copy constructor.
    NetworkOptions(const NetworkOptions &options);
    /// @brief Move constructor.
    NetworkOptions(NetworkOptions &&options) noexcept;

    /// @brief Sets the distance corrections.
    /// @param[in] corrections   The distance corrections.
    /// @throws std::invalid_argument if \c corrections.isInitialized() is false.
    void setDistanceCorrections(const Corrections::Distance &corrections);
    /// @result The distance corrections.
    /// @throws std::runtime_error \c hasDistanceCorrections() is false.
    [[nodiscard]] Corrections::Distance getDistanceCorrections() const;
    /// @result Ture indicates the distance corrections were set. 
    [[nodiscard]] bool hasDistanceCorrections() const noexcept; 

    /// @brief Sets the station corrections.
    /// @param[in] staitonsSet   The set of stations with corrections.
    /// @throws std::invalid_argument if stationsSet is empty.
    /// @note If an amplitude is given to the network-based calculator
    ///       and it does not have a corresponding correction then it will
    ///       not be used.
    void setStationCorrections(const Corrections::StationsSet &stationsSet);
    /// @result The station corrections.
    /// @throws std::runtime_error \c hasStationCorrections() is false.
    [[nodiscard]] Corrections::StationsSet getStationCorrections() const;
    /// @result True indicates the station corrections were set.
    [[nodiscard]] bool hasStationCorrections() const noexcept;

    /// @brief Sets the minimum number of station magnitudes to compute a
    ///        a network magnitude.
    /// @param[in] minimum   The minimum number of station magnitudes to
    ///                      compute a network magnitude.
    /// @throws std::invalid_argument if minimum is not positive.
    void setMinimumNumberOfStationMagnitudes(int minimum);
    /// @result The minimum number of station magnitudes to compute a network
    ///         magnitude.
    /// @note By default this is 2.
    [[nodiscard]] int getMinimumNumberOfStationMagnitudes() const noexcept;
    /// @brief Sets the strategy to combine station magnitudes into
    ///        a network magnitude.
    /// @param[in] strategy   The station magnitude combination strategy.
    void setStrategy(Strategy strategy) noexcept;
    /// @result The strategy to combine station magnitudes into a
    ///         network magnitude.
    [[nodiscard]] Strategy getStrategy() const noexcept;

    /// @brief Destructor.
    ~NetworkOptions();
 
    /// @brief Copy assignment.
    NetworkOptions& operator=(const NetworkOptions &options);
    /// @brief Move assignment.
    NetworkOptions& operator=(NetworkOptions &&options) noexcept;
private:
    class NetworkOptionsImpl;
    std::unique_ptr<NetworkOptionsImpl> pImpl;
};
}
#endif
