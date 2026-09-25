#include <stdexcept>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"

using namespace ULocalMagnitudeService::Corrections;
using ULocalMagnitudeService::Magnitude::StreamIdentifier;

TEST_CASE("ULocalMagnitudeService::Corrections::StationIdentifier",
          "[stationIdentifier]")
{
    SECTION("Defaults")
    {
        const StationIdentifier identifier;
        REQUIRE_FALSE(identifier.hasNetwork());
        REQUIRE_FALSE(identifier.hasStation());
        REQUIRE_THROWS_AS(identifier.getNetwork(), std::runtime_error);
        REQUIRE_THROWS_AS(identifier.getStation(), std::runtime_error);
        REQUIRE_THROWS_AS(identifier.toString(), std::runtime_error);
    }

    SECTION("Network and station")
    {
        StationIdentifier identifier;
        identifier.setNetwork("UU");
        identifier.setStation("CWU");
        REQUIRE(identifier.hasNetwork());
        REQUIRE(identifier.hasStation());
        REQUIRE(identifier.getNetwork() == "UU");
        REQUIRE(identifier.getStation() == "CWU");
        REQUIRE(identifier.toString() == "UU.CWU");
    }

    SECTION("Blanks are removed and names are capitalized")
    {
        StationIdentifier identifier;
        identifier.setNetwork(" w y ");
        identifier.setStation("\tymr\n");
        REQUIRE(identifier.getNetwork() == "WY");
        REQUIRE(identifier.getStation() == "YMR");
        REQUIRE(identifier.toString() == "WY.YMR");
        // Digits are untouched
        identifier.setStation("b206 ");
        REQUIRE(identifier.getStation() == "B206");
    }

    SECTION("Setting again overwrites")
    {
        StationIdentifier identifier;
        identifier.setNetwork("UU");
        identifier.setStation("CWU");
        identifier.setNetwork("WY");
        identifier.setStation("YMR");
        REQUIRE(identifier.toString() == "WY.YMR");
    }

    SECTION("Empty names are rejected")
    {
        StationIdentifier identifier;
        REQUIRE_THROWS_AS(identifier.setNetwork(""), std::invalid_argument);
        REQUIRE_THROWS_AS(identifier.setNetwork(" \t "),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(identifier.setStation(""), std::invalid_argument);
        REQUIRE_THROWS_AS(identifier.setStation(" \n "),
                          std::invalid_argument);
        REQUIRE_FALSE(identifier.hasNetwork());
        REQUIRE_FALSE(identifier.hasStation());
    }

    SECTION("A rejected name preserves the previous one")
    {
        StationIdentifier identifier;
        identifier.setNetwork("UU");
        identifier.setStation("CWU");
        REQUIRE_THROWS_AS(identifier.setNetwork("  "), std::invalid_argument);
        REQUIRE_THROWS_AS(identifier.setStation("  "), std::invalid_argument);
        REQUIRE(identifier.toString() == "UU.CWU");
    }

    SECTION("toString requires both network and station")
    {
        StationIdentifier networkOnly;
        networkOnly.setNetwork("UU");
        REQUIRE_THROWS_AS(networkOnly.toString(), std::runtime_error);

        StationIdentifier stationOnly;
        stationOnly.setStation("CWU");
        REQUIRE_THROWS_AS(stationOnly.toString(), std::runtime_error);
    }

    SECTION("From stream identifier")
    {
        StreamIdentifier streamIdentifier;
        streamIdentifier.setNetwork("UU");
        streamIdentifier.setStation("CWU");
        streamIdentifier.setChannel("HHE");
        streamIdentifier.setLocationCode("01");
        const StationIdentifier identifier{streamIdentifier};
        REQUIRE(identifier.getNetwork() == "UU");
        REQUIRE(identifier.getStation() == "CWU");
        REQUIRE(identifier.toString() == "UU.CWU");

        // Channel and location code aren't needed
        StreamIdentifier networkAndStation;
        networkAndStation.setNetwork("WY");
        networkAndStation.setStation("YMR");
        REQUIRE(StationIdentifier{networkAndStation}.toString() == "WY.YMR");
    }

    SECTION("From incomplete stream identifier")
    {
        StreamIdentifier noNetwork;
        noNetwork.setStation("CWU");
        noNetwork.setChannel("HHE");
        noNetwork.setLocationCode("01");
        REQUIRE_THROWS_AS(StationIdentifier{noNetwork}, std::invalid_argument);

        StreamIdentifier noStation;
        noStation.setNetwork("UU");
        noStation.setChannel("HHE");
        noStation.setLocationCode("01");
        REQUIRE_THROWS_AS(StationIdentifier{noStation}, std::invalid_argument);
    }

    SECTION("Copy and move")
    {
        StationIdentifier identifier;
        identifier.setNetwork("UU");
        identifier.setStation("CWU");

        // Copy constructor
        const StationIdentifier copy{identifier};
        REQUIRE(copy.toString() == "UU.CWU");

        // Copy is deep: modifying the original doesn't touch the copy
        identifier.setNetwork("WY");
        identifier.setStation("YMR");
        REQUIRE(copy.toString() == "UU.CWU");

        // Copy assignment
        StationIdentifier copyAssigned;
        copyAssigned = copy;
        REQUIRE(copyAssigned.toString() == "UU.CWU");

        // Move constructor
        StationIdentifier moved{std::move(copyAssigned)};
        REQUIRE(moved.toString() == "UU.CWU");

        // Move assignment
        StationIdentifier moveAssigned;
        moveAssigned = std::move(moved);
        REQUIRE(moveAssigned.toString() == "UU.CWU");
    }
}
