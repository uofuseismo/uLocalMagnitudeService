#ifndef ULOCAL_MAGNITUDE_SERVICE_GRPC_MAGNITUDE_SERVICE_OPTIONS_HPP
#define ULOCAL_MAGNITUDE_SERVICE_GRPC_MAGNITUDE_SERVICE_OPTIONS_HPP
#include <memory>
namespace ULocalMagnitudeService::Magnitude
{
 class NetworkMagnitudeCalculatorOptions;
}
namespace ULocalMagnitudeService::GRPC
{
 class ServerOptions;
}
namespace ULocalMagnitudeService::GRPC
{
/// @class MagnitudeServiceOptions magnitudeServiceOptions.hpp
/// @brief Sets the magnitude service options.
/// @copyright Ben Baker (University of Utah) distributed under the 
///            MIT NO AI license.
class MagnitudeServiceOptions
{
public:
    /// @brief Constructor.
    MagnitudeServiceOptions();
    /// @brief Copy constructor.
    MagnitudeServiceOptions(const MagnitudeServiceOptions &options);
    /// @brief Move constructor.
    MagnitudeServiceOptions(MagnitudeServiceOptions &&options) noexcept;

    /// @brief Sets the gRPC server options.
    /// @param[in] options   Options defining the undrelying gRPC service.
    /// @throws std::invalid_argument if options.validate() fails.
    void setGRPCOptions(const ServerOptions &options);
    /// @result The gRPC server options.
    /// @throws std::runtime_error if \c hasGRPCOptions() is false.
    [[nodiscard]] ServerOptions getGRPCOptions() const;
    /// @result True indicates the gRPC server options were set.
    [[nodiscard]] bool hasGRPCOptions() const noexcept;

    /// @brief Sets the network magnitude calculator options.
    /// @param[in] options   The network magnitude calculator options.
    /// @throws std::invalid_argument if optoins.validate() fails.
    void setNetworkMagnitudeCalculatorOptions(const Magnitude::NetworkMagnitudeCalculatorOptions &options);
    /// @result The network magnitude calculator options.
    /// @throws std::runtime_error if \c hasNetworkCalculatorOptions() is false.
    [[nodiscard]] Magnitude::NetworkMagnitudeCalculatorOptions getNetworkMagnitudeCalculatorOptions() const; 
    /// @result True indicates the network magnitude options were set.
    [[nodiscard]] bool hasNetworkMagnitudeCalculatorOptions() const noexcept;

    /// @brief Sets the maximum request message size in bytes.
    /// @param[in] maxMessageSize   The maximum message size in bytes.
    /// @throws std::invalid_argument if the request size is not positive.
    void setMaximumRequestMessageSizeInBytes(int maxMessageSize);
    /// @result The maximum message size in bytes.
    /// @note By default this is 4096.
    [[nodiscard]] int getMaximumRequestMessageSizeInBytes() const noexcept;
    
    /// @brief Destructor
    ~MagnitudeServiceOptions();
    /// @brief Copy assignment.
    MagnitudeServiceOptions& operator=(const MagnitudeServiceOptions &options);
    /// @brief Move assignment.
    MagnitudeServiceOptions& operator=(MagnitudeServiceOptions &&options) noexcept;
private:
    class MagnitudeServiceOptionsImpl;
    std::unique_ptr<MagnitudeServiceOptionsImpl> pImpl;
}; 
}
#endif
