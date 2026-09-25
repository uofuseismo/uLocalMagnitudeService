#ifndef ULOCAL_MAGNITUDE_CORRECTIONS_STATION_OPTIONS_HPP
#define ULOCAL_MAGNITUDE_CORRECTIONS_STATION_OPTIONS_HPP
#include <memory>
namespace ULocalMagnitude::Corrections
{
 class StationIdentifier;
}
namespace ULocalMagnitude::Corrections
{
/// @class StationOptions stationOptions.hpp
/// @brief Defines the options defining a station (site) correction.
/// @copyright Ben Baker (University of Utah) distributed under the
///            MIT NO AI license.
class StationOptions
{
public:
    /// @brief Constructor.
    StationOptions();
    /// @brief Copy constructor.
    StationOptions(const StationOptions &options);
    /// @brief Move constructor.
    StationOptions(StationOptions &&options) noexcept;

    /// @brief Sets the station identifier.
    /// @param[in] stationIdentifier  The station identifier (network code
    ///                               and name).
    /// @throws std::invalid_argument if the network or station isn't set.
    void setIdentifier(const StationIdentifier &identifier);
    /// @result The station identifier.
    /// @throws std::runtime_error if \c hasIdentifier() is false.
    [[nodiscard]] StationIdentifier getIdentifier() const;
    /// @result True indicates the station identifier was set.
    [[nodiscard]] bool hasIdentifier() const noexcept;
   
    /// @brief The magnitude correction to add to the station magnitude.
    /// @param[in] correction  The correction in magnitude units.
    void setCorrection(double correction) noexcept;
    /// @result The correction in magnitude units.
    /// @throws std::runtime_error if \c hasCorrection() is false.
    [[nodiscard]] double getCorrection() const;
    /// @result True indicates the correciton was set.
    [[nodiscard]] bool hasCorrection() const noexcept;

    /// @brief Destructor.
    ~StationOptions();
    /// @brief Copy assignment.
    StationOptions& operator=(const StationOptions &options);
    /// @brief Move assignment.
    StationOptions& operator=(StationOptions &&options) noexcept;
private:
    class StationOptionsImpl;
    std::unique_ptr<StationOptionsImpl> pImpl;
};
}
#endif
