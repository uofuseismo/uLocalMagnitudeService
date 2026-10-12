#ifndef ULOCAL_MAGNITUDE_SERVICE_WAVEFORM_PROCESSING_PACKET_HPP
#define ULOCAL_MAGNITUDE_SERVICE_WAVEFORM_PROCESSING_PACKET_HPP
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>
namespace ULocalMagnitudeService::WaveformProcessing
{
class Packet
{
public:
    /// @brief Constructor.
    Packet();
    /// @brief Copy constructor.
    Packet(const Packet &packet);
    /// @brief Move constructor.
    Packet(Packet &&packet) noexcept;
    /// @brief Creates the packet from a protobuf message.
    template<typename U> explicit Packet(const U &packet);

    /// @brief Sets the sampling rate.
    /// @param[in] samplingRate   The sampling rate in Hz.
    /// @throws std::invalid_argument if this is not positive.
    void setSamplingRate(double samplingRate);
    /// @result The sampling rate in Hz.
    /// @throws std::runtime_error if \c hasSamplingRate() is false.
    [[nodiscard]] double getSamplingRate() const;
    /// @result True indicates that the sampling rate was set.
    [[nodiscard]] bool hasSamplingRate() const noexcept;

    /// @brief Sets the start time of the packet (time of first sample).
    /// @param[in] startTime   The start time (UTC) of the packet.
    void setStartTime(const std::chrono::nanoseconds &startTime) noexcept;
    /// @result The start time (UTC) of the packet.
    /// @throws std::runtime_error if \c hasStartTime() is false.
    [[nodiscard]] std::chrono::nanoseconds getStartTime() const; 
    /// @result True indicates that the start time was set.
    [[nodiscard]] bool hasStartTime() const noexcept;
    /// @result The end time (UTC) of the packet.
    /// @throws std::runtime_error if \c hasStartTime() is false,
    ///         \c hasSamplingRate() is false, or \c hasData() is false.
    [[nodiscard]] std::chrono::nanoseconds getEndTime() const;
   
    /// @brief Sets the data from a string (bytes) representation.
    /// @param[in] data   The seismic data.  This is a container for protobuf
    ///                   bytes data and is assumed to be in little endian
    ///                   order.
    /// @throws std::invalid_argument if the data is empty or is an invalid size
    ///         (e.g., data.size()%sizeof(U) != 0 or sizeof(U)*nSamples != data.size()).
    template<typename U> void setData(const std::string &data, const uint32_t nSamples);
    /// @result The data.
    /// @throws std::runtime_error if \c hasData() is false.
    [[nodiscard]] std::vector<double> getData() const;
    /// @result A reference to the data.
    /// @throws std::runtime_error if \c hasData() is false.
    /// @note This exists for optimization purposes and you should prefer \c getData().
    [[nodiscard]] const std::vector<double> &getDataReference() const;
    /// @result True indicates that the data was set.
    [[nodiscard]] bool hasData() const noexcept;

    /// @brief Destructor.
    ~Packet();
    /// @brief Copy assignmet.
    Packet& operator=(const Packet &packet);
    /// @brief Move assignment.
    Packet& operator=(Packet &&packet) noexcept;
private:
    class PacketImpl;
    std::unique_ptr<PacketImpl> pImpl;
};
}
#endif
