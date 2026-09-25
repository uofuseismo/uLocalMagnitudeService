#ifndef ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_STATION_IDENTIFIER_HPP
#define ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_STATION_IDENTIFIER_HPP
#include <memory>
#include <string>
namespace ULocalMagnitudeService::Magnitude
{
 class StreamIdentifier;
}
namespace ULocalMagnitudeService::Corrections
{
/// @class StationIdentifier stationIdentifier.hpp
/// @brief Uniquely defines a station in terms of network code and
///        station name.
/// @copyright Ben Baker (University of Utah) distributed under the 
///            MIT NO AI license.
class StationIdentifier
{
public:
    /// @brief Constructor.
    StationIdentifier();
    /// @brief Copy constructor.
    StationIdentifier(const StationIdentifier &identifier);
    /// @brief Move constructor.
    StationIdentifier(StationIdentifier &&identifier) noexcept;
    /// @brief Constructs a station identifier from a stream identifier.
    /// @param[in] identifier  The stream identifier from which to create this
    ///                        station identifier.
    /// @throws std::invalid_argument if the \c identifier.hasNetwork() or
    ///         \c identifier.hasStation() is false.
    explicit StationIdentifier(const Magnitude::StreamIdentifier &identifier);

    /// @brief Sets the network code.
    /// @param[in] network   The network code - e.g., UU.  
    /// @throws std::invalid_argument if the network code is empty.
    /// @note Blanks will be removed and this will be capitalized.
    void setNetwork(const std::string &network);
    /// @result The network code.
    [[nodiscard]] std::string getNetwork() const;
    /// @result True indicates the network was set.
    [[nodiscard]] bool hasNetwork() const noexcept;
 
    /// @brief Sets the station name.
    /// @param[in] station   The station name - e.g., CWU.
    /// @throws std::invalid_argument if the station name is empty.
    /// @note Blanks will be removed and this will be capitalized.
    void setStation(const std::string &station);
    /// @result The station name.
    [[nodiscard]] std::string getStation() const;
    /// @result True indicates the station was set.
    [[nodiscard]] bool hasStation() const noexcept;

    /// @result String representation - e.g. "UU.CWU".
    /// @throws std::runtime_erorr if \c hasStation() or \c hasNetwork() is false.
    [[nodiscard]] std::string toString() const;

    /// @brief Destructor.
    ~StationIdentifier();
    /// @brief Copy assignment.
    StationIdentifier& operator=(const StationIdentifier &identifier);
    /// @brief Move assignment.
    StationIdentifier& operator=(StationIdentifier &&identifier) noexcept;
private:
    class StationIdentifierImpl;
    std::unique_ptr<StationIdentifierImpl> pImpl;
};
}
#endif
