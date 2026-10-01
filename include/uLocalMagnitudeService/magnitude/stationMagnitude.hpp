#ifndef ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_STATION_MAGNITUDE_HPP
#define ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_STATION_MAGNITUDE_HPP
#include <memory>
#include <string>
namespace ULocalMagnitudeService::Magnitude
{
/// @class StationMagnitude stationMagnitude.hpp
/// @brief A station magnitude and the corrections that went into it.
///        The station magnitude is computed as
///          log10(AverageAmplitude/2) + C_d + C_s
///        where C_d is the distance correction and C_s the station
///        correction.  The corrections are reported so that a client
///        need not recompute them.
/// @copyright Ben Baker (University of Utah) distributed under the
///            MIT NO AI license.
class StationMagnitude
{
public:
    /// @brief Constructor.
    StationMagnitude();
    /// @brief Copy constructor.
    StationMagnitude(const StationMagnitude &magnitude);
    /// @brief Move constructor.
    StationMagnitude(StationMagnitude &&magnitude) noexcept;

    /// @brief Sets the station magnitude.  This includes the station and
    ///        distance corrections.
    /// @param[in] value  The station magnitude.
    /// @throws std::invalid_argument if the value is not finite.
    void setValue(double value);
    /// @result The station magnitude.
    /// @throws std::runtime_error if \c hasValue() is false.
    [[nodiscard]] double getValue() const;
    /// @result True indicates the station magnitude was set.
    [[nodiscard]] bool hasValue() const noexcept;

    /// @brief Sets the station correction that was applied.
    /// @param[in] correction  The station correction in magnitude units.
    /// @throws std::invalid_argument if the correction is not finite.
    void setStationCorrection(double correction);
    /// @result The station correction in magnitude units.
    /// @throws std::runtime_error if \c hasStationCorrection() is false.
    [[nodiscard]] double getStationCorrection() const;
    /// @result True indicates the station correction was set.
    [[nodiscard]] bool hasStationCorrection() const noexcept;

    /// @brief Sets the distance correction that was applied.
    /// @param[in] correction  The distance correction in magnitude units.
    /// @throws std::invalid_argument if the correction is not finite.
    void setDistanceCorrection(double correction);
    /// @result The distance correction in magnitude units.
    /// @throws std::runtime_error if \c hasDistanceCorrection() is false.
    [[nodiscard]] double getDistanceCorrection() const;
    /// @result True indicates the distance correction was set.
    [[nodiscard]] bool hasDistanceCorrection() const noexcept;

    /// @brief Sets the name of the station.
    /// @param[in] name  The station name - e.g., UU.CWU.
    /// @throws std::invalid_argument if the name is empty.
    void setStationName(const std::string &name);
    /// @result The station name - e.g., UU.CWU.
    /// @throws std::runtime_error if \c hasStationName() is false.
    [[nodiscard]] std::string getStationName() const;
    /// @result True indicates the station name was set.
    [[nodiscard]] bool hasStationName() const noexcept;

    /// @brief Destructor.
    ~StationMagnitude();
    /// @brief Copy assignment.
    StationMagnitude& operator=(const StationMagnitude &magnitude);
    /// @brief Move assignment.
    StationMagnitude& operator=(StationMagnitude &&magnitude) noexcept;
private:
    class StationMagnitudeImpl;
    std::unique_ptr<StationMagnitudeImpl> pImpl;
};
}
#endif
