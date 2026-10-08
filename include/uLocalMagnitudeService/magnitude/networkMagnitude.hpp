#ifndef ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_NETWORK_MAGNITUDE_HPP
#define ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_NETWORK_MAGNITUDE_HPP
#include <memory>
#include <utility>
#include <vector>
namespace ULocalMagnitudeService::Magnitude
{
 class Observation;
 class Residual;
}
namespace ULocalMagnitudeService::Magnitude
{
/// @class NetworkMagnitude networkMagnitude.hpp
/// @brief The network magnitude is the aggregation of individual station
///        magnitudes and the number reported in a catalog. 
/// @class Ben Baker (University of Utah) distributed under
///        the MIT NO AI license.
class NetworkMagnitude 
{
public:
    /// @brief Constructor.
    NetworkMagnitude();
    /// @brief Copy constructor.
    NetworkMagnitude(const NetworkMagnitude &magnitude);
    /// @brief Move constructor.
    NetworkMagnitude(NetworkMagnitude &&magnitude) noexcept;

    /// @brief Sets the network magnitude.
    /// @param[in] value   The network magnitude.
    /// @throws std::invalid_argument if this less than -10 or greater than 10.
    void setValue(double value);
    /// @result The network magnitude.
    /// @throws std::runtime_error if \c hasValue() is false.
    [[nodiscard]] double getValue() const;
    /// @result True indicates that the netork magnitude was set.
    [[nodiscard]] bool hasValue() const noexcept;

    /// @brief Sets the observations and residuals.
    /// @param[in] observationsAndResiduals  Each element is an observation and
    ///                                      corresponding (magnitude) residual.
    void setObservationsAndResiduals(const std::vector<std::pair<Observation, Residual>> &observationsAndResiduals);
    /// @result True indicates that the observations and residuals were set.
    [[nodiscard]] bool hasObservationsAndResiduals() const noexcept;

    /// @brief Destructor.
    ~NetworkMagnitude();
    /// @brief Copy assginment.
    NetworkMagnitude& operator=(const NetworkMagnitude &magnitude);
    /// @brief Move assignment.
    NetworkMagnitude& operator=(NetworkMagnitude &&magnitude) noexcept;
private:
    class NetworkMagnitudeImpl;
    std::unique_ptr<NetworkMagnitudeImpl> pImpl;
};
}
#endif
