#include <stdexcept>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include "uLocalMagnitude/corrections/stationIdentifier.hpp"

using namespace ULocalMagnitude::Corrections;

TEST_CASE("ULocalMagnitude::Corrections::StationIdentifier",
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
