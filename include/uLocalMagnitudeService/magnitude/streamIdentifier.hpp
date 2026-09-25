#ifndef ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_STREAM_IDENTIFIER_HPP
#define ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_STREAM_IDENTIFIER_HPP
#include <memory>
#include <string>
namespace ULocalMagnitudeService::Magnitude
{
/// @class StreamIdentifier streamIdentifier.hpp
/// @brief Uniquely defines a stream on which an amplitude was computed
///        in terms of network code, station name, channel code,
///        location code.
/// @copyright Ben Baker (University of Utah) distributed under the 
///            MIT NO AI license.
class StreamIdentifier
{
public:
    /// @brief Constructor.
    StreamIdentifier();
    /// @brief Copy constructor.
    StreamIdentifier(const StreamIdentifier &identifier);
    /// @brief Move constructor.
    StreamIdentifier(StreamIdentifier &&identifier) noexcept;

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

    /// @brief Sets the channel name.
    /// @param[in] channel   The channel name - e.g., HHE.
    /// @throws std::invalid_argument if the channel name is empty.
    /// @note Blanks will be removed and this will be capitalized.
    void setChannel(const std::string &channel);
    /// @result The channel name.
    [[nodiscard]] std::string getChannel() const;
    /// @result True indicates the channel was set.
    [[nodiscard]] bool hasChannel() const noexcept;

    /// @brief Sets the location code.
    /// @param[in] locationCode   The location code e.g., 01.
    /// @note A location code "  " will be converted to "".
    void setLocationCode(const std::string &locationCode);
    /// @result The location code.
    [[nodiscard]] std::string getLocationCode() const;
    /// @result True indicates the location code was set.
    [[nodiscard]] bool hasLocationCode() const noexcept;

    /// @result String representation - e.g. "UU.CWU.HHE.01" or "NN.PRN.HHN".
    /// @throws std::runtime_error if \c hasStation(), \c hasNetwork(),
    ///         \c hasChannel(), or \c hasLocationCode()  is false.
    [[nodiscard]] std::string toString() const;

    /// @brief Destructor.
    ~StreamIdentifier();
    /// @brief Copy assignment.
    StreamIdentifier& operator=(const StreamIdentifier &identifier);
    /// @brief Move assignment.
    StreamIdentifier& operator=(StreamIdentifier &&identifier) noexcept;
private:
    class StreamIdentifierImpl;
    std::unique_ptr<StreamIdentifierImpl> pImpl;
};
}
#endif
