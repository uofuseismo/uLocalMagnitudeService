#ifndef ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_STATIONS_HPP
#define ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_STATIONS_HPP
#include <expected>
#include <map>
#include <memory>
#include <string>
namespace ULocalMagnitudeService::Corrections
{
 class StationsSet;
 class StationIdentifier;
}
namespace ULocalMagnitudeService::Corrections
{
/// @class Stations stations.hpp
/// @brief Defines the corrections for a collection of stations (sites) in
///        a network.
/// @copyright Ben Baker (University of Utah) distributed under the
///            MIT NO AI license.
class Stations
{
public:
    enum class ErrorCode
    {
        InvalidStation,      /*!< The station name is invalid - e.g., empty. */
        StationDoesNotExist, /*!< No correction exists for this station. */
        Uninitialized        /*!< The corrections class is not initialized. */
    };
public:
    /// @brief Defines the correction for each station.
    explicit Stations(const StationsSet &corrections);
    /// @brief Copy constructor.
    Stations(const Stations &stations);
    /// @brief Move constructor.
    Stations(Stations &&station) noexcept;

    /// @result True indicates this class is initialized.
    [[nodiscard]] bool isInitialized() const noexcept;

    /// @param[in] identifier   The station (site) identifier.
    /// @result The magnitude to add to the station magnitude.  Necessarily,
    ///         this is in magnitudes units.
    /// @note This is the preferred route for computing the correction.
    [[nodiscard]] auto operator()(const StationIdentifier &identifier) const -> std::expected<double, ErrorCode>;
    /// @param[in] station   The name of station/site - e.g., UU.CWU.
    /// @result The magnitude to add to the station magnitude.  Necessarily,
    ///         this is in magnitudes units.
    [[nodiscard]] auto operator()(const std::string &station) const -> std::expected<double, ErrorCode>;

    /// @result The station corrections map.
    /// @throws std::runtime_error if \c isInitialized() is false.
    [[nodiscard]] std::map<std::string, double> getCorrections() const;
 
    /// @brief Destructor.
    ~Stations();
    /// @brief Copy assignment.
    Stations& operator=(const Stations &stations);
    /// @brief Move assignment.
    Stations& operator=(Stations &&stations) noexcept;
private:
    class StationsImpl;
    std::unique_ptr<StationsImpl> pImpl;
};
}
#endif
