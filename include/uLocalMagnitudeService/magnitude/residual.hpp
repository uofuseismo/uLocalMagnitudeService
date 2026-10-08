#ifndef ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_RESIDUAL_HPP
#define ULOCAL_MAGNITUDE_SERVICE_MAGNITUDE_RESIDUAL_HPP
#include <memory>
#include <string>
namespace ULocalMagnitudeService::Magnitude
{
 class StationMagnitude;
}
namespace ULocalMagnitudeService::Magnitude
{
/// @class Residual residual.hpp
/// @brief Defines the residual for an observation used in creating the network
////       magnitude.
/// @copyright Ben Baker (University of Utah) distributed under the
///            MIT NO AI license.
class Residual
{
public:
    /// @brief Constructor.
    Residual();
    /// @brief Copy constructor.
    Residual(const Residual &residual);
    /// @brief Move constructor.
    Residual(Residual &&residual) noexcept;

    /// @brief Sets the residual, i.e., the NetworkMagnitude - StationMagnitude.
    /// @param[in] residual   The magnitude residual.  This has magnitude units.
    void setValue(double residual) noexcept;
    /// @result The residual in magnitude units.
    /// @throws std::runtime_error if \c hasValue() is false.
    [[nodiscard]] double getValue() const;
    /// @result True indicates that the residual was set. 
    [[nodiscard]] bool hasValue() const noexcept;

    /// @brief Destructor.
    ~Residual();
    /// @brief Copy assignment.
    Residual& operator=(const Residual &residual);
    /// @brief Move assignment.
    Residual& operator=(Residual &&residual) noexcept;
private:
    class ResidualImpl;
    std::unique_ptr<ResidualImpl> pImpl;
};
}
#endif
