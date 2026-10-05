#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include "uLocalMagnitudeService/grpc/magnitudeServiceOptions.hpp"
#include "uLocalMagnitudeService/grpc/serverOptions.hpp"
#include "uLocalMagnitudeService/magnitude/networkMagnitudeCalculatorOptions.hpp"

using namespace ULocalMagnitudeService::GRPC;

class MagnitudeServiceOptions::MagnitudeServiceOptionsImpl
{
public:
    Magnitude::NetworkMagnitudeCalculatorOptions
        mNetworkMagnitudeCalculatorOptions;
    ServerOptions mGRPCServerOptions;
    int mMaximumRequestMessageSizeInBytes{4096};
    bool mHasNetworkMagnitudeCalculatorOptions{false};
    bool mHasGRPCServerOptions{false};
};

/// Constructor
MagnitudeServiceOptions::MagnitudeServiceOptions() :
    pImpl(std::make_unique<MagnitudeServiceOptionsImpl> ())
{
}

/// Copy constructor
MagnitudeServiceOptions::MagnitudeServiceOptions(
    const MagnitudeServiceOptions &options)
{
    *this = options;
}

/// Move constructor
MagnitudeServiceOptions::MagnitudeServiceOptions(
    MagnitudeServiceOptions &&options) noexcept
{
    *this = std::move(options);
}

/// Copy assignment
MagnitudeServiceOptions& 
MagnitudeServiceOptions::operator=(const MagnitudeServiceOptions &options)
{
    if (&options == this){return *this;}
    pImpl = std::make_unique<MagnitudeServiceOptionsImpl> (*options.pImpl);
    return *this;
}

/// Move assignment
MagnitudeServiceOptions& 
MagnitudeServiceOptions::operator=(MagnitudeServiceOptions &&options) noexcept
{
    if (&options == this){return *this;}
    pImpl = std::move(options.pImpl);
    return *this;
}

/// Destructor
MagnitudeServiceOptions::~MagnitudeServiceOptions() = default;

/// gRPC server
void MagnitudeServiceOptions::setGRPCOptions(const ServerOptions &options)
{
    try
    {
        options.validate();
    }
    catch (const std::exception &e)
    {
        throw std::invalid_argument("Cannot set gRPC server options because "
                                  + std::string {e.what()});
    }
    pImpl->mGRPCServerOptions = options;
    pImpl->mHasGRPCServerOptions = true;
}

ServerOptions MagnitudeServiceOptions::getGRPCOptions() const
{
    if (!hasGRPCOptions())
    {
        throw std::runtime_error("gRPC server options not set");
    }
    return pImpl->mGRPCServerOptions;
}

bool MagnitudeServiceOptions::hasGRPCOptions() const noexcept
{
    return pImpl->mHasGRPCServerOptions;
} 

/// Network magnitude calculator options
void MagnitudeServiceOptions::setNetworkMagnitudeCalculatorOptions(
    const Magnitude::NetworkMagnitudeCalculatorOptions &options)
{
    try
    {
        options.validate();
    }
    catch (const std::exception &e)
    {
        throw std::invalid_argument(
           "Cannot set magnitude calcalator options because "
         + std::string {e.what()} );
    }
    pImpl->mNetworkMagnitudeCalculatorOptions = options;
    pImpl->mHasNetworkMagnitudeCalculatorOptions = true;
}

ULocalMagnitudeService::Magnitude::NetworkMagnitudeCalculatorOptions
MagnitudeServiceOptions::getNetworkMagnitudeCalculatorOptions() const
{
    if (!hasNetworkMagnitudeCalculatorOptions())
    {
        throw std::invalid_argument(
           "Network magnitude calculator options not set");
    }
    return pImpl->mNetworkMagnitudeCalculatorOptions;
}

bool MagnitudeServiceOptions::hasNetworkMagnitudeCalculatorOptions() 
    const noexcept
{
    return pImpl->mHasNetworkMagnitudeCalculatorOptions;
}

/// Maximum request message size
void MagnitudeServiceOptions::setMaximumRequestMessageSizeInBytes( 
    const int maximumMessageSize)
{
    if (maximumMessageSize <= 0)
    {
        throw std::invalid_argument(
            "Maximum request message size must be positive");
    }
    pImpl->mMaximumRequestMessageSizeInBytes = maximumMessageSize;
}

int MagnitudeServiceOptions::getMaximumRequestMessageSizeInBytes()
    const noexcept
{
    return pImpl->mMaximumRequestMessageSizeInBytes;
}

