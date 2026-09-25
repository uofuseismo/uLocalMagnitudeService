#ifndef ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_STATIONS_SET_HPP
#define ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_STATIONS_SET_HPP
#include <map>
#include <memory>
#include <optional>
#include <string>
namespace ULocalMagnitudeService::Corrections
{
 class Station;
}
namespace ULocalMagnitudeService::Corrections
{
/// @class StationsSet stationsSet.hpp
/// @brief Defines a collection of station corrections.
/// @copyright Ben Baker (University of Utah) distributed under the
///            MIT NO AI license.
class StationsSet
{
public:
    /// @brief Constructor.
    StationsSet();
    /// @brief Copy constructor.
    StationsSet(const StationsSet &set);
    /// @brief Move constructor.
    StationsSet(StationsSet &&set) noexcept;

    /// @brief Inserts a station correction.
    /// @result True if the correction was inserted.  False if the 
    ///         station correction already exists and overwrite is false.
    /// @throws std::invalid_argument if the correction is not initialized.
    [[nodiscard]] bool insert(const Station &correction, bool overwrite = false);

    /// @result True indicates that there are no corrections.
    [[nodiscard]] bool empty() const noexcept;

    /// @result The list stations in the set.
    [[nodiscard]] std::map<std::string, Station> getCorrections() const;

    /// @param[in] identifier  A station identifier - e.g., "UU.CWU".
    /// @result Gets a correction corresponding to the given correction.
    ///         If this is null then then the station with the given
    ///         identifier could not be found.
    [[nodiscard]] std::optional<Station> getCorrection(const std::string &identifier) const;

    /// @brief Destructor.
    ~StationsSet();
    /// @brief Copy assignment.
    StationsSet& operator=(const StationsSet &set);
    /// @brief Move assignment.
    StationsSet& operator=(StationsSet &&set) noexcept;
private:
    class StationsSetImpl;
    std::unique_ptr<StationsSetImpl> pImpl;
};
}
#endif

