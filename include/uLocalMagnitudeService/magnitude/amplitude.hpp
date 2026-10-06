#ifndef ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_AMPLITUDE_HPP
#define ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_AMPLITUDE_HPP
#include <memory>
#include <string>
namespace ULocalMagnitudeService::Magnitude
{
 class StreamIdentifier;
}
namespace ULocalMagnitudeService::Magnitude
{
/// @class Amplitude amplitude.hpp
/// @brief Defines an observed amplitude.
/// @copyright Ben Baker (University of Utah) distributed under the
///            MIT NO AI license.
class Amplitude
{
public:
    /// @brief Constructor.
    Amplitude();
    /// @brief Copy constructor.
    Amplitude(const Amplitude &amplitude);
    /// @brief Move constructor.
    Amplitude(Amplitude &&amplitude) noexcept;
    /// @brief Creates the amplitude from a protobuf definition.
    /// @throws std::invalid_argument if the stream identifier is invalid
    ///         or the amplitude value is not postive and not finite.
    /// @note Supported types: ULocalMagnitudeServiceAPI::V1::Magnitude::Amplitude.
    ///       Any other type fails to link.
    template<typename U>
    explicit Amplitude(const U &amplitude);

    /// @brief Defines the amplitude in millimeters.
    /// @param[in] amplitude  The amplitude value.
    /// @throws std::invalid_argument if the amplitude is not positive.
    void setValue(double amplitude);
    /// @result The amplitude in millimeters.
    /// @throws std::runtime_error if \c hasAmplitude() is false.
    [[nodiscard]] double getValue() const;
    /// @result True indicates that the amplitude was set.
    [[nodiscard]] bool hasValue() const noexcept;

    /// @brief Defines the stream identifier.
    /// @param[in,out] identifier  The stream identifier.  On exit,
    ///                            stream identifier's behavior is
    ///                            undefined.
    /// @throws std::invalid_argument if the network, station name,
    ///         channel code, or location code is not set.
    void setIdentifier(StreamIdentifier &&identifier);
    /// @brief Defines the stream identifier.
    /// @param[in] identifier  The stream identifier.
    /// @throws std::invalid_argument if the network, station name,
    ///         channel code, or location code is not set.
    void setIdentifier(const StreamIdentifier &identifier); 
    /// @result The stream identifier.
    /// @throws std::runtime_error if \c hasIdentifier() is false.
    [[nodiscard]] StreamIdentifier getIdentifier() const;
    /// @result The stream name on which the amplitude was made - e.g., UU.CWU.HHE.01.
    /// @throws std::runtime_error if \c hasIdentifier() is false.
    [[nodiscard]] std::string getName() const;
    /// @result The stream identifier.
    [[nodiscard]] bool hasIdentifier() const noexcept; 

    /// @brief Creates the amplitude message from this class.
    /// @result The amplitude in the desired protobuf message format. 
    /// @throws std::runtime_error if  \c hasIdentifier() or \c hasValue() is false.
    /// @note Supported types: ULocalMagnitudeServiceAPI::V1::Magnitude::Amplitude.
    ///       Any other type fails to link.
    template<typename U> [[nodiscard]] U toMessage() const;

    /// @brief Destructor.
    ~Amplitude();
    /// @brief Copy assignment.
    Amplitude& operator=(const Amplitude &amplitude);
    /// @brief Move assignment.
    Amplitude& operator=(Amplitude &&amplitude) noexcept;
private:
    class AmplitudeImpl;
    std::unique_ptr<AmplitudeImpl> pImpl;
};
}
#endif
