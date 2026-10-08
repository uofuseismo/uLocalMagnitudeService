#ifndef ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_OBSERVATION_HPP
#define ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_OBSERVATION_HPP
#include <memory>
#include <string>
#include <utility>
namespace ULocalMagnitudeService::Magnitude
{
 class Amplitude;
}
namespace ULocalMagnitudeService::Magnitude
{
/// @class Observation observation.hpp
/// @brief Comprises a station observation.  This is a pair of amplitudes
///        measured on different channels on the same station.
/// @copyright Ben Baker (University of Utah) distributed under the
///            MIT NO AI license.
class Observation
{
public:
    /// @brief Constructor.
    Observation();
    /// @brief Copy constructor.
    Observation(const Observation &observation);
    /// @brief Move constructor.
    Observation(Observation &&observation) noexcept;
    /// @brief Creates the observation class from a proto file.
    /// @throws std::invalid_argument if the station amplitudes
    ///         are not set, the amplitudes are invalid (e.g., values
    ///         are not positive or finite, units are unknown, or
    ///         the streams do not map to complimentary channels),
    ///         or the epicentral distance is negative.  Additionally,
    ///         throws std::invalid_argument if the hypocenter lacks
    ///         a finite latitude, longitude, depth, or the latitude
    ///         or depth exceed the usable ranges.
    /// @note Supported types: ULocalMagnitudeServiceAPI::V1::Magnitude::StationAmplitudeMeasurement for U and
    ///                        ULocalMagnitudeServiceAPI::V1::Magnitude::Hypocenter for V.
    ///       Any other type fails to link.
    template<typename U, typename V>
    Observation(const U &stationAmplitudeMeasurement, const V &hypocenter);


    /// @brief Sets an amplitude pair. 
    /// @param[in] amplitudes  The measured amplitudes on the non-vertical
    ///                        channels.
    /// @throws std::invalid_argument if amplitudes correspond to the same
    ///         stream, either amplitude is missing an identifier or value,
    ///         or the identifiers indicate a sensor mismatch - e.g.,
    ///         an amplitude on UU.CWU.HHE.01 and UU.CWU.ENN.01 or 
    ///         US.DUG.HH1.00 and US.DUG.HH2.02.
    void setAmplitudes(const std::pair<Amplitude, Amplitude> &amplitudes);
    /// @brief Sets an amplitude pair.
    /// @param[in,out] amplitudes  The measured amplitudes on the non-vertical
    ///                            channels.  On exit, the behavior of
    ///                            amplitudes is undefined.
    /// @throws std::invalid_argument if amplitudes correspond to the same
    ///         stream, either amplitude is missing an identifier or value,
    ///         or the identifiers indicate a sensor mismatch - e.g.,
    ///         an amplitude on UU.CWU.HHE.01 and UU.CWU.ENN.01 or 
    ///         US.DUG.HH1.00 and US.DUG.HH2.02.
    void setAmplitudes(std::pair<Amplitude, Amplitude> &&amplitudes);
    /// @result The amplitudes.
    /// @throws std::runtime_error if \c hasAmplitudes() is false.
    [[nodiscard]] std::pair<Amplitude, Amplitude> getAmplitudes() const;
    /// @result A reference to the amplitudes. 
    /// @note This exists for performance reasons; \c getAmplitudes() should
    ///       be preferred.
    [[nodiscard]] const std::pair<Amplitude, Amplitude> &getAmplitudesReference() const;
    /// @result True indicates the amplitudes were set.
    [[nodiscard]] bool hasAmplitudes() const noexcept;
    /// @result The station name - e.g., UU.CWU.
    /// @throws std::runtime_error if \c hasAmplitudes() is false.
    [[nodiscard]] std::string getStationName() const;

    /// @brief Sets the source-receiver epicentral distance in meters. 
    /// @param[in] distance  The great-circle source receiver distance
    ///                      in meters.
    /// @throws std::invalid_argument if the distance is negative or exceeds
    ///         21,000 km. 
    void setEpicentralDistance(double distance);
    /// @result The source-receiver epicentral distance in meters.
    /// @throws std::runtime_error if \c hasEpicentralDistance() is false.
    [[nodiscard]] double getEpicentralDistance() const;
    /// @result True indicates that the source-receiver distance was set.
    [[nodiscard]] bool hasEpicentralDistance() const noexcept;

    /// @brief Sets the event depth in meters.
    /// @param[in] depth   The event depth in meters.
    /// @throws std::invalid_argument if this is less than -8600 or greater than
    ///         900000.
    void setDepth(double depth);
    /// @result The source depth in meters.
    /// @throws std::runtime_error if \c hasDepth() is false.
    [[nodiscard]] double getDepth() const;
    /// @result True indicates that the source depth was set.
    [[nodiscard]] bool hasDepth() const noexcept;

    /// @brief Destructor.
    ~Observation();
    /// @brief Copy assignment.
    Observation& operator=(const Observation &observation);
    /// @brief Move assignment.
    Observation& operator=(Observation &&observation) noexcept; 
private:
    class ObservationImpl;
    std::unique_ptr<ObservationImpl> pImpl;
};
}
#endif
