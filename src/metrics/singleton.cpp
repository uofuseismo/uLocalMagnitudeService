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

void Singleton::incrementUnauthenticatedCounter(const std::string &route)
{
    if (route.empty()){return;}
    {   
    const std::scoped_lock lock{mMutex};
    auto it = mUnauthenticatedCounterMap.find(route);
    if (it != mUnauthenticatedCounterMap.end())
    {   
        it->second = it->second + 1;
        return;
    }
    mUnauthenticatedCounterMap.insert( {route, 1} );
    }   
}

std::map<std::string, int64_t> Singleton::getUnauthenticatedCounters() const
{
    const std::scoped_lock lock{mMutex};
    return mUnauthenticatedCounterMap;
}

/// Server error
void Singleton::incrementServerErrorCounter(const std::string &route)
{
    if (route.empty()){return;}
    {   
    const std::scoped_lock lock{mMutex};
    auto it = mServerErrorCounterMap.find(route);
    if (it != mServerErrorCounterMap.end())
    {   
        it->second = it->second + 1;
        return;
    }   
    mServerErrorCounterMap.insert( {route, 1} );
    }   
}

std::map<std::string, int64_t> Singleton::getServerErrorCounters() const
{
    const std::scoped_lock lock{mMutex};
    return mServerErrorCounterMap;
}

/// Client error
void Singleton::incrementClientErrorCounter(const std::string &route)
{
    if (route.empty()){return;}
    {   
    const std::scoped_lock lock{mMutex};
    auto it = mClientErrorCounterMap.find(route);
    if (it != mClientErrorCounterMap.end())
    {   
        it->second = it->second + 1;
        return;
    }   
    mClientErrorCounterMap.insert( {route, 1} );
    }   
}

std::map<std::string, int64_t> Singleton::getClientErrorCounters() const
{
    const std::scoped_lock lock{mMutex};
    return mClientErrorCounterMap;
}

/// Route duration
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

