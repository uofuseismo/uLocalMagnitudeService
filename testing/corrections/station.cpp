#include <stdexcept>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitudeService/corrections/station.hpp"
#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"
#include "uLocalMagnitudeService/corrections/stationOptions.hpp"

using namespace ULocalMagnitudeService::Corrections;

TEST_CASE("ULocalMagnitudeService::Corrections::Station", "[station]")
{
    StationIdentifier identifier;
    identifier.setNetwork("WY");
    identifier.setStation("YFT");
    constexpr double correction{0.18};

    StationOptions options;
    options.setIdentifier(identifier);
    options.setCorrection(correction);

    SECTION("From options")
    {
        const Station station{options};
        REQUIRE(station.isInitialized());
        REQUIRE(station.getName() == "WY.YFT");
        REQUIRE_THAT(station(), Catch::Matchers::WithinAbs(correction, 1.e-14));
    }

    SECTION("Options are copied in")
    {
        // Changing the options afterwards doesn't leak into the station
        const Station station{options};
        StationIdentifier otherIdentifier;
        otherIdentifier.setNetwork("UU");
        otherIdentifier.setStation("CWU");
        options.setIdentifier(otherIdentifier);
        options.setCorrection(-0.25);
        REQUIRE(station.getName() == "WY.YFT");
        REQUIRE_THAT(station(), Catch::Matchers::WithinAbs(correction, 1.e-14));
    }

    SECTION("Incomplete options are rejected")
    {
        REQUIRE_THROWS_AS(Station {StationOptions {}}, std::invalid_argument);

        StationOptions identifierOnly;
        identifierOnly.setIdentifier(identifier);
        REQUIRE_THROWS_AS(Station {identifierOnly}, std::invalid_argument);

        StationOptions correctionOnly;
        correctionOnly.setCorrection(correction);
        REQUIRE_THROWS_AS(Station {correctionOnly}, std::invalid_argument);
    }

    SECTION("Copy and move")
    {
        const auto check = [&](const Station &result)
        {
            REQUIRE(result.isInitialized());
            REQUIRE(result.getName() == "WY.YFT");
            REQUIRE_THAT(result(),
                         Catch::Matchers::WithinAbs(correction, 1.e-14));
        };

        const Station station{options};

        // Copy constructor
        // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
        const Station copy{station};
        check(copy);

        // Copy assignment
        StationOptions otherOptions;
        StationIdentifier otherIdentifier;
        otherIdentifier.setNetwork("UU");
        otherIdentifier.setStation("CWU");
        otherOptions.setIdentifier(otherIdentifier);
        otherOptions.setCorrection(-0.25);
        Station copyAssigned{otherOptions};
        copyAssigned = copy;
        check(copyAssigned);

        // Move constructor
        Station moved{std::move(copyAssigned)};
        check(moved);

        // Move assignment
        Station moveAssigned{otherOptions};
        moveAssigned = std::move(moved);
        check(moveAssigned);
    }
}
