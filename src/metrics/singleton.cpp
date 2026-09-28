#include "uLocalMagnitudeService/metrics/singleton.hpp"

using namespace ULocalMagnitudeService::Metrics;

Singleton &Singleton::getInstance()
{
    static Singleton instance;
    return instance;
}

/// Initialize
void ULocalMagnitudeService::Metrics::initializeSingleton()
{
    Singleton::getInstance();
}

