#include <stdexcept>
#include <string>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitude/corrections/stationIdentifier.hpp"
#include "uLocalMagnitude/corrections/stationOptions.hpp"

using namespace ULocalMagnitude::Corrections;

namespace
{
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
StationIdentifier makeIdentifier(const std::string &network,
                                 const std::string &station)
{
    StationIdentifier identifier;
    identifier.setNetwork(network);
    identifier.setStation(station);
    return identifier;
}
}

TEST_CASE("ULocalMagnitude::Corrections::StationOptions", "[stationOptions]")
{
    const auto identifier = makeIdentifier("WY", "YFT");
    constexpr double correction{0.18};

    SECTION("Defaults")
    {
        const StationOptions options;
        REQUIRE_FALSE(options.hasIdentifier());
        REQUIRE_FALSE(options.hasCorrection());
        REQUIRE_THROWS_AS(options.getIdentifier(), std::runtime_error);
        REQUIRE_THROWS_AS(options.getCorrection(), std::runtime_error);
    }

    SECTION("Identifier and correction")
    {
        StationOptions options;
        options.setIdentifier(identifier);
        options.setCorrection(correction);
        REQUIRE(options.hasIdentifier());
        REQUIRE(options.hasCorrection());
        const auto identifierBack = options.getIdentifier();
        REQUIRE(identifierBack.getNetwork() == "WY");
        REQUIRE(identifierBack.getStation() == "YFT");
        REQUIRE(identifierBack.toString() == "WY.YFT");
        REQUIRE_THAT(options.getCorrection(),
                     Catch::Matchers::WithinAbs(correction, 1.e-14));
    }

    SECTION("Identifier is copied in")
    {
        // Changing the caller's identifier afterwards doesn't leak in
        StationOptions options;
        auto mutableIdentifier = identifier;
        options.setIdentifier(mutableIdentifier);
        mutableIdentifier.setStation("YMR");
        REQUIRE(options.getIdentifier().toString() == "WY.YFT");
    }

    SECTION("Incomplete identifier is rejected")
    {
        StationOptions options;
        REQUIRE_THROWS_AS(options.setIdentifier(StationIdentifier {}),
                          std::invalid_argument);

        StationIdentifier networkOnly;
        networkOnly.setNetwork("WY");
        REQUIRE_THROWS_AS(options.setIdentifier(networkOnly),
                          std::invalid_argument);

        StationIdentifier stationOnly;
        stationOnly.setStation("YFT");
        REQUIRE_THROWS_AS(options.setIdentifier(stationOnly),
                          std::invalid_argument);

        REQUIRE_FALSE(options.hasIdentifier());
    }

    SECTION("Rejected identifier preserves the previous one")
    {
        StationOptions options;
        options.setIdentifier(identifier);
        REQUIRE_THROWS_AS(options.setIdentifier(StationIdentifier {}),
                          std::invalid_argument);
        REQUIRE(options.getIdentifier().toString() == "WY.YFT");
    }

    SECTION("Correction can be zero or negative")
    {
        StationOptions options;
        options.setCorrection(0);
        REQUIRE(options.hasCorrection());
        REQUIRE(options.getCorrection() == 0);
        options.setCorrection(-0.18);
        REQUIRE_THAT(options.getCorrection(),
                     Catch::Matchers::WithinAbs(-0.18, 1.e-14));
    }

    SECTION("Setting again overwrites")
    {
        StationOptions options;
        options.setIdentifier(identifier);
        options.setCorrection(correction);
        options.setIdentifier(makeIdentifier("UU", "CWU"));
        options.setCorrection(-0.25);
        REQUIRE(options.getIdentifier().toString() == "UU.CWU");
        REQUIRE_THAT(options.getCorrection(),
                     Catch::Matchers::WithinAbs(-0.25, 1.e-14));
    }

    SECTION("Copy and move")
    {
        StationOptions options;
        options.setIdentifier(identifier);
        options.setCorrection(correction);

        const auto check = [&](const StationOptions &result)
        {
            REQUIRE(result.getIdentifier().toString() == "WY.YFT");
            REQUIRE_THAT(result.getCorrection(),
                         Catch::Matchers::WithinAbs(correction, 1.e-14));
        };

        // Copy constructor
        const StationOptions copy{options};
        check(copy);

        // Copy is deep: modifying the original doesn't touch the copy
        options.setIdentifier(makeIdentifier("UU", "CWU"));
        options.setCorrection(-0.25);
        check(copy);

        // Copy assignment
        StationOptions copyAssigned;
        copyAssigned = copy;
        check(copyAssigned);

        // Move constructor
        StationOptions moved{std::move(copyAssigned)};
        check(moved);

        // Move assignment
        StationOptions moveAssigned;
        moveAssigned = std::move(moved);
        check(moveAssigned);
    }
}
