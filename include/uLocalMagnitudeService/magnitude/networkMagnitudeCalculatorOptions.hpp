#ifndef ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_NETWORK_MAGNITUDE_CALCULATOR_OPTIONS_HPP
#define ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_NETWORK_MAGNITUDE_CALCULATOR_OPTIONS_HPP
#include <memory>
namespace ULocalMagnitudeService::Corrections
{
 class Distance;
 class StationsSet;
}
namespace ULocalMagnitudeService::Magnitude
{
/// @class NetworkMagnitudeCalculatorOptions networkMagnitudeCalculatorOptions.hpp
/// @brief Defines the options for computing a network magnitude.
/// @copyright Ben Baker (University of Utah) distributed under the
///            MIT NO AI license.
class NetworkMagnitudeCalculatorOptions
{
public:
    enum class Strategy
    {
        Average  /*!< Computes a magnitude via a straight average. */
    };
public:
    /// @brief Constructor.
    NetworkMagnitudeCalculatorOptions();
    /// @brief Copy constructor.
    NetworkMagnitudeCalculatorOptions(const NetworkMagnitudeCalculatorOptions &options);
    /// @brief Move constructor.
    NetworkMagnitudeCalculatorOptions(NetworkMagnitudeCalculatorOptions &&options) noexcept;

    /// @brief Sets the distance corrections.
    /// @param[in] corrections   The distance corrections.
    /// @throws std::invalid_argument if \c corrections.isInitialized() is false.
    void setDistanceCorrections(const Corrections::Distance &corrections);
    /// @result The distance corrections.
    /// @throws std::runtime_error \c hasDistanceCorrections() is false.
    [[nodiscard]] Corrections::Distance getDistanceCorrections() const;
    /// @result A reference to the distance corrections.
    /// @throws std::runtime_error \c hasDistanceCorrections() is false.
    /// @note This exists as an optimization and
    ///       \c getDistanceCorrections() should be preferred.
    [[nodiscard]] const Corrections::Distance &getDistanceCorrectionsReference() const;
    /// @result True indicates the distance corrections were set. 
    [[nodiscard]] bool hasDistanceCorrections() const noexcept; 

    /// @brief Sets the station corrections.
    /// @param[in] stationsSet   The set of stations with corrections.
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
    ///        network magnitude.
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

    /// @brief Quick way to validate that the network magnitude calculator
    ///        options are valid in aggregate.
    /// @throws std::runtime_error if the station corrections or distance
    ///         corrections are not set. 
    void validate() const;

    /// @brief Destructor.
    ~NetworkMagnitudeCalculatorOptions();
 
    /// @brief Copy assignment.
    NetworkMagnitudeCalculatorOptions& operator=(const NetworkMagnitudeCalculatorOptions &options);
    /// @brief Move assignment.
    NetworkMagnitudeCalculatorOptions& operator=(NetworkMagnitudeCalculatorOptions &&options) noexcept;
private:
    class NetworkMagnitudeCalculatorOptionsImpl;
    std::unique_ptr<NetworkMagnitudeCalculatorOptionsImpl> pImpl;
};
}
#endif
