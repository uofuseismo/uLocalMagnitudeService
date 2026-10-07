#ifndef ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_STATION_MAGNITUDE_HPP
#define ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_STATION_MAGNITUDE_HPP
#include <memory>
#include <string>
#include <utility>
namespace ULocalMagnitudeService::Magnitude
{
 class Amplitude;
}
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

    /// @brief Sets the amplitudes that went into building the station magnitude.
    /// @param[in] amplitudes  The measured amplitudes on the non-vertical
    ///                        channels.
    /// @throws std::invalid_argument if amplitudes correspond to the same
    ///         stream, either amplitude is missing an identifier or value,
    ///         or the identifiers indicate a sensor mismatch - e.g.,
    ///         an amplitude on UU.CWU.HHE.01 and UU.CWU.ENN.01 or 
    ///         US.DUG.HH1.00 and US.DUG.HH2.02.
    void setAmplitudes(const std::pair<Amplitude, Amplitude> &amplitudes);
    /// @result The amplitudes that went into making the station magnitude.
    /// @throws std::runtime_error if \c hasAmplitudes() is false.
    [[nodiscard]] std::pair<Amplitude, Amplitude> getAmplitudes() const;
    /// @result True indicates that the amplitudes were set.
    [[nodiscard]] bool hasAmplitudes() const noexcept;
    /// @result The station name - e.g., UU.CWU.
    /// @throws std::runtime_error if \c hasAmplitudes() is false.
    [[nodiscard]] std::string getStationName() const;

    /// @brief Creates the station magnitude message from this class.
    /// @result The station magnitude in the desired protobuf message format. 
    /// @throws std::runtime_error if \c hasValue(), \c hasStationCorrection(),
    ///         \c hasDistanceCorrection(), or \c hasAmplitudes()  is false.
    /// @note Supported types: ULocalMagnitudeServiceAPI::V1::Magnitude::StationMagnitude
    ///       Any other type fails to link.
    template<typename U> [[nodiscard]] U toMessage() const;

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
