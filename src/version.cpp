#include <string>
#include "uLocalMagnitude/version.hpp"

using namespace ULocalMagnitude;

int Version::getMajor() noexcept
{
    return uLocalMagnitude_MAJOR;
}

int Version::getMinor() noexcept
{
    return uLocalMagnitude_MINOR;
}

int Version::getPatch() noexcept
{
    return uLocalMagnitude_PATCH;
}

//NOLINTBEGIN(bugprone-easily-swappable-parameters)
bool Version::isAtLeast(const int major, const int minor,
                        const int patch) noexcept
//NOLINTEND(bugprone-easily-swappable-parameters)
{
    if (uLocalMagnitude_MAJOR < major){return false;}
    if (uLocalMagnitude_MAJOR > major){return true;}
    if (uLocalMagnitude_MINOR < minor){return false;}
    if (uLocalMagnitude_MINOR > minor){return true;}
    if (uLocalMagnitude_PATCH < patch){return false;}
    return true;
}

std::string Version::getVersion() noexcept
{
    std::string version{uLocalMagnitude_VERSION};
    return version;
}

std::string Version::getTag() noexcept
{
    std::string tag{uLocalMagnitude_GITTAG};
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
