#ifndef ULOCAL_MAGNITUDE_SERVICE_SINGLETON_HPP
#define ULOCAL_MAGNITUDE_SERVICE_SINGLETON_HPP
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <map>
namespace ULocalMagnitudeService::Metrics
{
/// @class Singleton singleton.hpp
/// @brief A globally accessible point from which to read and write application
///        metrics.
/// @note This should be instantiated at application startup.
///       @sa \initializeSingleton().
/// @copyright Ben Baker (University of Utah) distributed under the
///            MIT NO AI license.
class Singleton
{
public:
    /// @result An instance of the singleton.
    [[maybe_unused]] static Singleton &getInstance();

    /// @brief Increments the number of successful RPCs.
    void incrementSuccessCounter(const std::string &route);
    /// @result The success counts for each RPC. 
    std::map<std::string, int64_t> getSuccessCounters() const;

    /// @brief Increments the number of unauthenticated access attempts to an RPC.
    void incrementUnauthenticatedCounter(const std::string &route); 
    /// @result The unauthenticated access attempts for each RPC.
    std::map<std::string, int64_t> getUnauthenticatedCounters() const;

    /// @brief Increments the number of server errors in an RPC.
    void incrementServerErrorCounter(const std::string &route); 
    /// @result The server errors for each RPC.
    std::map<std::string, int64_t> getServerErrorCounters() const;

    /// @brief Sets the route recorder.
    void setRouteRecorderCallback(const std::function<void (const std::chrono::duration<double> &,
                                                            const std::string &)> &callback);
    /// @brief Logs the route duration.
    void recordRouteDuration(const std::chrono::duration<double> &duration,
                             const std::string &routeName);

    /// @brief Resets the counters an dutilization.  This is useful for unit tests.
    void resetMetrics() noexcept;
private:
    Singleton() = default;
    ~Singleton() = default;
    mutable std::mutex mMutex;
    std::map<std::string, int64_t> mServerErrorCounterMap;     // 500 response codes
    std::map<std::string, int64_t> mClientErrorCounterMap;     // 400 response codes
    std::map<std::string, int64_t> mUnauthenticatedCounterMap; // 401 response code
    std::map<std::string, int64_t> mSuccessCounterMap;         // 200 response codes
    bool mHaveDurationCallback{false};
};
/// @brief Initializes the metrics singleton once and for all.  This is to be
///        used at application start up.
void initializeSingleton();
}
#endif
