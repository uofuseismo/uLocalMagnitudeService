#ifndef ULOCAL_MAGNITUDE_CORRECTIONS_STATION_HPP
#define ULOCAL_MAGNITUDE_CORRECTIONS_STATION_HPP
#include <memory>
#include <string>

namespace ULocalMagnitude::Corrections
{
 class StationOptions;
}

namespace ULocalMagnitude::Corrections
{
/// @class Station station.hpp
/// @brief Defines a station (site) magnitude correction.
/// @copyright Ben Baker (University of Utah) distributed under the
///            MIT NO AI license.
class Station
{
public:
    /// @brief Constructor. 
    /// @param[in] options  Defines the station correction options.
    /// @throws std::invalid_argument if the identifier or correction is not set.
    explicit Station(const StationOptions &options);
    /// @brief Copy constructor.
    Station(const Station &station);
    /// @brief Move constructor. 
    Station(Station &&station) noexcept;

    /// @result True indicates teh class is initialized.
    [[nodiscard]] bool isInitialized() const noexcept;

    /// @result The station (site) correction.  This is in magnititude units.
    /// @throws std::runtime_error if \c isInitialized() is false.
    [[nodiscard]] double operator()() const;
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
