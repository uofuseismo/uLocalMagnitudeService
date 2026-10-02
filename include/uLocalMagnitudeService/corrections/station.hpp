#ifndef ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_STATION_HPP
#define ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_STATION_HPP
#include <expected>
#include <memory>
#include <string>

namespace ULocalMagnitudeService::Corrections
{
 class StationOptions;
}

namespace ULocalMagnitudeService::Corrections
{
/// @class Station station.hpp
/// @brief Defines a station (site) magnitude correction.
/// @copyright Ben Baker (University of Utah) distributed under the
///            MIT NO AI license.
class Station
{
public:
    enum class ErrorCode
    {
        Uninitialized         /*!< The calculator is not initialized. */
    }; 
public:
    /// @brief Constructor. 
    /// @param[in] options  Defines the station correction options.
    /// @throws std::invalid_argument if the identifier or correction is not set.
    explicit Station(const StationOptions &options);
    /// @brief Copy constructor.
    Station(const Station &station);
    /// @brief Move constructor. 
    Station(Station &&station) noexcept;

    /// @result True indicates the class is initialized.
    [[nodiscard]] bool isInitialized() const noexcept;

    /// @result The station (site) correction.  This is in magnititude units.
    [[nodiscard]] auto operator()() const noexcept -> std::expected<double, ErrorCode>;
    /// @result The station name.
    /// @throws std::runtime_error if \c isInitialized() is false.
    [[nodiscard]] std::string getName() const;

    /// @brief Destructor.
    ~Station(); 

    /// @brief Copy assignment.
    Station& operator=(const Station &station);
    /// @brief Move assignment.
    Station& operator=(Station &&station) noexcept;

    Station() = delete;
private:
    class StationImpl;
    std::unique_ptr<StationImpl> pImpl;
};
}
#endif
