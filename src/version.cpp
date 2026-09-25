#include <string>
#include "uLocalMagnitudeService/version.hpp"

using namespace ULocalMagnitudeService;

int Version::getMajor() noexcept
{
    return uLocalMagnitudeService_MAJOR;
}

int Version::getMinor() noexcept
{
    return uLocalMagnitudeService_MINOR;
}

int Version::getPatch() noexcept
{
    return uLocalMagnitudeService_PATCH;
}

//NOLINTBEGIN(bugprone-easily-swappable-parameters)
bool Version::isAtLeast(const int major, const int minor,
                        const int patch) noexcept
//NOLINTEND(bugprone-easily-swappable-parameters)
{
    if (uLocalMagnitudeService_MAJOR < major){return false;}
    if (uLocalMagnitudeService_MAJOR > major){return true;}
    if (uLocalMagnitudeService_MINOR < minor){return false;}
    if (uLocalMagnitudeService_MINOR > minor){return true;}
    if (uLocalMagnitudeService_PATCH < patch){return false;}
    return true;
}

std::string Version::getVersion() noexcept
{
    std::string version{uLocalMagnitudeService_VERSION};
    return version;
}

std::string Version::getTag() noexcept
{
    std::string tag{uLocalMagnitudeService_GITTAG};
    return tag;
}

std::string Version::getVersionWithTag() noexcept
{
    auto tag = Version::getTag();
    if (tag.empty())
    {
        return Version::getVersion();
    }
    else
    {
        return Version::getVersion() + "-" + tag;
    }
}
