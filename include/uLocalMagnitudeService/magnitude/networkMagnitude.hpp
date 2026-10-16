#ifndef ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_NETWORK_MAGNITUDE_HPP
#define ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_NETWORK_MAGNITUDE_HPP
#include <memory>
#include <tuple>
#include <utility>
#include <vector>
#include <uLocalMagnitudeService/magnitude/networkMagnitudeCalculator.hpp>

namespace ULocalMagnitudeService::Magnitude
{
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

    /// @brief Sets the residuals.
    /// @param[in,out] residuals  The residuals (Network - StationMagnitude)
    ///                           and the corresponding observation number.
    /// @throws std::invalid_argument if any residual does not have a value
    ///         or a station magnitude.
    void setResiduals(std::vector<std::pair<Residual, int>> &&stationMagnitudesAndResiduals);
    void setResiduals(const std::vector<std::pair<Residual, int>> &stationMagnitudesAndResiduals);
    /// @result The residuals and corresponding observation numbers.
    /// @throws std::runtime_error if \c hasResiduals() is false.
    [[nodiscard]] std::vector<std::pair<Residual, int>> getResiduals() const;
    [[nodiscard]] const std::vector<std::pair<Residual, int>> &getResidualsReference() const;
    /// @result True indicates that the residuals were set.
    [[nodiscard]] bool hasResiduals() const noexcept;
    /// @result The station magnitudes and residual message.
    /// @throws std::runtime_error if \c hasResiduals() is false.
    /// @note This is defined for ULocalMagnitudeServiceAPI::Magnitude::V1::NetworkMagnitudeFromAmplitudesResponse::StationMagnitudeAndResidual.
    //template<typename U> std::vector<U> toStationMagnitudesAndResiduals() const;


    /// @brief Sets the reasons for rejected observations.
    /// @note The rejected observations vector can be empty.
    void setRejectedObservations(const std::vector<std::pair<NetworkMagnitudeCalculator::StationCalculatorErrorCode, int>> &rejectedObservations); 
    /// @result The rejeted observations.
    /// @throws std::runtime_error if \c hasRejectedObservations() is false.
    [[nodiscard]] std::vector<std::pair<NetworkMagnitudeCalculator::StationCalculatorErrorCode, int>> getRejectedObservations() const;
    /// @result True indicates the rejected observations were set.
    [[nodiscard]] bool hasRejectedObservations() const noexcept;
    /// @result The rejected observations message.
    /// @throws std::runtime_error if \c hasRejectedObservations() is false.
    /// @note This is defined for ULocalMagnitudeServiceAPI::Magnitude::V1::NetworkMagnitudeFromAmplitudesResponse::RejectedObservation.
    template<typename U> std::vector<U> toRejectedObservationsMessage() const;

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
