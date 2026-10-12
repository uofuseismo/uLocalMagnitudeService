#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <google/protobuf/util/time_util.h>
#include "uLocalMagnitudeService/waveformProcessing/packet.hpp"
#include <uLocalMagnitudeServiceAPI/v1/waveformProcessing/data_type.pb.h>
#include <uLocalMagnitudeServiceAPI/v1/waveformProcessing/packet.pb.h>

using namespace ULocalMagnitudeService::WaveformProcessing;

namespace
{
template<typename T>
std::vector<double> unpack(const std::string &data,
                           const uint32_t nSamples,
                           const bool swapBytes)
{
    if (data.empty())
    {
        throw std::invalid_argument("Data is empty");
    }
    constexpr auto dataTypeSize{sizeof(T)};
    if (data.size()%dataTypeSize != 0)
    {
        throw std::invalid_argument("Data must be divisible by "
                                  + std::to_string(dataTypeSize));
    }
    auto expectedSize = sizeof(T)*static_cast<T> (nSamples);
    if (data.size() != expectedSize)
    {
        throw std::invalid_argument("Data size must be "
                                  + std::to_string(expectedSize));
    }
    std::vector<double> result(nSamples);
    union CharacterValueUnion
    {
        unsigned char cArray[dataTypeSize];
        T value;
    };
    CharacterValueUnion cvUnion;
    if (swapBytes)
    {
        for (uint32_t i = 0; i < nSamples; ++i)
        {
            auto j = i*dataTypeSize;
            std::reverse_copy(data.data() + j, data.data() + j + dataTypeSize,
                              cvUnion.cArray);
            result[i] = cvUnion.value;
        }
    }
    else
    {
        for (uint32_t i = 0; i < nSamples; ++i)
        {
            auto j = i*dataTypeSize;
            std::copy(data.data() + j, data.data() + j + dataTypeSize,
                      cvUnion.cArray);
            result[i] = cvUnion.value;
        }
    }
    return result;
}

}

class Packet::PacketImpl
{
public:
    void updateEndTime()
    {
        if (!mHasStartTime){return;}
        if (!mHasData){return;}
        if (mSamplingRate <= 0){return;}
        auto samplingPeriodNanoSeconds
            = static_cast<int64_t> (std::round(1.e9/mSamplingRate));
        auto nSamples = static_cast<int64_t> (mData.size());
        auto endTime = mStartTime.count()
                     + samplingPeriodNanoSeconds*(nSamples - 1);
        mEndTime = std::chrono::nanoseconds {mEndTime};
    } 
    std::vector<double> mData;
    std::chrono::nanoseconds mStartTime;
    std::chrono::nanoseconds mEndTime;
    double mSamplingRate{0};
    bool mHasStartTime{false};
    bool mHasData{false};
};

/// Constructor
Packet::Packet() :
    pImpl(std::make_unique<PacketImpl> ())
{
}

/// Copy constructor
Packet::Packet(const Packet &packet)
{
    *this = packet;
}

/// Move constructor
Packet::Packet(Packet &&packet) noexcept
{
    *this = std::move(packet);
}

/// Copy assignment
Packet& Packet::operator=(const Packet &packet)
{
    if (&packet == this){return *this;}
    pImpl = std::make_unique<PacketImpl> (*packet.pImpl);
    return *this;
}

/// Move assignment
Packet& Packet::operator=(Packet &&packet) noexcept
{
    if (&packet == this){return *this;}
    pImpl = std::move(packet.pImpl);
    return *this;
}

/// Construct from message
template<>
Packet::Packet(
    const ULocalMagnitudeServiceAPI::V1::WaveformProcessing::Packet &packet)
{
    if (!packet.has_start_time())
    {
        throw std::invalid_argument("Start time not set");
    }
    if (!packet.has_sampling_rate())
    {
        throw std::invalid_argument("Sampling rate not set");
    }
    if (!packet.has_data_type())
    {
        throw std::invalid_argument("Data type not set");
    }
    if (!packet.has_data())
    {
        throw std::runtime_error("Data not set");
    }
    Packet thisPacket;
    auto startTime
        = google::protobuf::util::TimeUtil::TimestampToNanoseconds(
            packet.start_time());
    thisPacket.setStartTime(std::chrono::nanoseconds {startTime});
    thisPacket.setSamplingRate(packet.sampling_rate());
    namespace UAPI = ULocalMagnitudeServiceAPI::V1::WaveformProcessing;
    auto dataType = packet.data_type();
    auto nSamples = packet.number_of_samples();
    if (dataType == UAPI::DataType::DATA_TYPE_UNKNOWN)
    {
        throw std::invalid_argument("Cannot handle unknown data type");
    }
    else if (dataType == UAPI::DataType::DATA_TYPE_INTEGER_32)
    {
        thisPacket.setData<int32_t> (packet.data(), nSamples);
    }
    else if (dataType == UAPI::DataType::DATA_TYPE_FLOAT)
    {
        thisPacket.setData<float> (packet.data(), nSamples);
    } 
    else if (dataType == UAPI::DataType::DATA_TYPE_DOUBLE)
    {
        thisPacket.setData<double> (packet.data(), nSamples);
    }
    else if (dataType == UAPI::DataType::DATA_TYPE_INTEGER_64)
    {
        thisPacket.setData<int64_t> (packet.data(), nSamples);
    }
    else
    {
        throw std::invalid_argument("Unhandled data type");
    }
    *this = std::move(thisPacket);
}

/// Destructor
Packet::~Packet() = default;

/// Data
template<typename U>
void Packet::setData(const std::string &data, const uint32_t nSamples)
{
    if (data.empty())
    {
        throw std::invalid_argument("Data is empty");
    }
    auto swapBytes = std::endian::native == std::endian::little ? false : true;
    pImpl->mData = ::unpack<U>(data, nSamples, swapBytes); 
    pImpl->mHasData = true;
    pImpl->updateEndTime();
}

std::vector<double> Packet::getData() const
{
    if (!hasData())
    {
        throw std::runtime_error("Data not set");
    }
    return pImpl->mData;
}

const std::vector<double> &Packet::getDataReference() const
{
    if (!hasData())
    {
        throw std::runtime_error("Data not set");
    }
    return *&pImpl->mData;
}

bool Packet::hasData() const noexcept
{
    return pImpl->mHasData;
}

/// Start time
void Packet::setStartTime(const std::chrono::nanoseconds &startTime) noexcept
{ 
    pImpl->mStartTime = startTime;
    pImpl->mHasStartTime = true;
    pImpl->updateEndTime();
}

std::chrono::nanoseconds Packet::getStartTime() const
{
    if (!hasStartTime())
    {
        throw std::runtime_error("Start time not set");
    }
    if (!hasSamplingRate())
    {
        throw std::runtime_error("Sampling rate not set");
    }
    if (!hasData())
    {
        throw std::runtime_error("Data not set");
    }
    return pImpl->mEndTime;
}

bool Packet::hasStartTime() const noexcept
{
    return pImpl->mHasStartTime;
}

std::chrono::nanoseconds Packet::getEndTime() const
{
    if (!hasStartTime()){throw std::runtime_error("Start time not set");}
    if (!hasSamplingRate()){throw std::runtime_error("Sampling rate not set");}
    if (!hasData()){throw std::runtime_error("Data not set");}
    return pImpl->mEndTime;
}

/// Sampling rate
void Packet::setSamplingRate(const double samplingRate)
{
    if (!std::isfinite(samplingRate))
    {
        throw std::invalid_argument("Sampling rate not finite");
    }
    if (samplingRate <= 0)
    {
        throw std::invalid_argument("Sampling rate is not positive");
    }
    pImpl->mSamplingRate = samplingRate;
    pImpl->updateEndTime();
}

double Packet::getSamplingRate() const
{
    if (!hasSamplingRate())
    {
        throw std::runtime_error("Sampling rate not set");
    }
    return pImpl->mSamplingRate;
}

bool Packet::hasSamplingRate() const noexcept
{
    return pImpl->mSamplingRate > 0 ? true : false;
}

///--------------------------------------------------------------------------///
///                              Template instantiation                      ///
///--------------------------------------------------------------------------/// 

template void ULocalMagnitudeService::WaveformProcessing::Packet::setData<int32_t>(const std::string &, uint32_t );
template void ULocalMagnitudeService::WaveformProcessing::Packet::setData<int64_t>(const std::string &, uint32_t );
template void ULocalMagnitudeService::WaveformProcessing::Packet::setData<float>(const std::string &, uint32_t );
template void ULocalMagnitudeService::WaveformProcessing::Packet::setData<double>(const std::string &, uint32_t );
