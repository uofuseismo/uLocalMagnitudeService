#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include "uLocalMagnitudeService/metrics/singleton.hpp"

using namespace ULocalMagnitudeService::Metrics;

Singleton &Singleton::getInstance()
{
    static Singleton instance;
    return instance;
}

/// Success counts 
void Singleton::incrementSuccessCounter(const std::string &route)
{
    if (route.empty()){return;}
    {   
    const std::scoped_lock lock{mMutex};
    auto it = mSuccessCounterMap.find(route);
    if (it != mSuccessCounterMap.end())
    {   
        it->second = it->second + 1;
        return;
    }   
    mSuccessCounterMap.insert( {route, 1} );
    }   
}

std::map<std::string, int64_t> Singleton::getSuccessCounters() const
{
    const std::scoped_lock lock{mMutex};
    return mSuccessCounterMap;
}

void Singleton::recordRouteDuration(
    const std::chrono::duration<double> &duration,
    const std::string &routeName)
{
    if (mHaveDurationCallback)
    {
    }
}

/// Initialize
void ULocalMagnitudeService::Metrics::initializeSingleton()
{
    Singleton::getInstance();
}

