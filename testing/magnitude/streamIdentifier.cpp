#include <stdexcept>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"

using namespace ULocalMagnitudeService::Magnitude;

TEST_CASE("ULocalMagnitudeService::Magnitude::StreamIdentifier",
          "[streamIdentifier]")
{
    SECTION("Defaults")
    {
        const StreamIdentifier identifier;
        REQUIRE_FALSE(identifier.hasNetwork());
        REQUIRE_FALSE(identifier.hasStation());
        REQUIRE_FALSE(identifier.hasChannel());
        REQUIRE_FALSE(identifier.hasLocationCode());
        REQUIRE_THROWS_AS(identifier.getNetwork(), std::runtime_error);
        REQUIRE_THROWS_AS(identifier.getStation(), std::runtime_error);
        REQUIRE_THROWS_AS(identifier.getChannel(), std::runtime_error);
        REQUIRE_THROWS_AS(identifier.getLocationCode(), std::runtime_error);
        REQUIRE_THROWS_AS(identifier.toString(), std::runtime_error);
    }

    SECTION("Network, station, channel, and location code")
    {
        StreamIdentifier identifier;
        identifier.setNetwork("UU");
        identifier.setStation("CWU");
        identifier.setChannel("HHE");
        identifier.setLocationCode("01");
        REQUIRE(identifier.hasNetwork());
        REQUIRE(identifier.hasStation());
        REQUIRE(identifier.hasChannel());
        REQUIRE(identifier.hasLocationCode());
        REQUIRE(identifier.getNetwork() == "UU");
        REQUIRE(identifier.getStation() == "CWU");
        REQUIRE(identifier.getChannel() == "HHE");
        REQUIRE(identifier.getLocationCode() == "01");
        REQUIRE(identifier.toString() == "UU.CWU.HHE.01");
    }

    SECTION("Blank location code")
    {
        StreamIdentifier identifier;
        identifier.setNetwork("NN");
        identifier.setStation("PRN");
        identifier.setChannel("HHN");
        identifier.setLocationCode("  ");
        REQUIRE(identifier.hasLocationCode());
        REQUIRE(identifier.getLocationCode().empty());
        REQUIRE(identifier.toString() == "NN.PRN.HHN");
        identifier.setLocationCode("");
        REQUIRE(identifier.hasLocationCode());
        REQUIRE(identifier.toString() == "NN.PRN.HHN");
    }

    SECTION("Blanks are removed and names are capitalized")
    {
        StreamIdentifier identifier;
        identifier.setNetwork(" w y ");
        identifier.setStation("\tymr\n");
        identifier.setChannel(" e h z");
        identifier.setLocationCode(" 0 1 ");
        REQUIRE(identifier.getNetwork() == "WY");
        REQUIRE(identifier.getStation() == "YMR");
        REQUIRE(identifier.getChannel() == "EHZ");
        REQUIRE(identifier.getLocationCode() == "01");
        REQUIRE(identifier.toString() == "WY.YMR.EHZ.01");
        // Digits are untouched
        identifier.setStation("b206 ");
        REQUIRE(identifier.getStation() == "B206");
        identifier.setLocationCode("a0");
        REQUIRE(identifier.getLocationCode() == "A0");
    }

    SECTION("Setting again overwrites")
    {
        StreamIdentifier identifier;
        identifier.setNetwork("UU");
        identifier.setStation("CWU");
        identifier.setChannel("HHE");
        identifier.setLocationCode("01");
        identifier.setNetwork("WY");
        identifier.setStation("YMR");
        identifier.setChannel("HHZ");
        identifier.setLocationCode("02");
        REQUIRE(identifier.toString() == "WY.YMR.HHZ.02");
    }

    SECTION("Empty names are rejected")
    {
        StreamIdentifier identifier;
        REQUIRE_THROWS_AS(identifier.setNetwork(""), std::invalid_argument);
        REQUIRE_THROWS_AS(identifier.setNetwork(" \t "),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(identifier.setStation(""), std::invalid_argument);
        REQUIRE_THROWS_AS(identifier.setStation(" \n "),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(identifier.setChannel(""), std::invalid_argument);
        REQUIRE_THROWS_AS(identifier.setChannel("  "),
                          std::invalid_argument);
        REQUIRE_FALSE(identifier.hasNetwork());
        REQUIRE_FALSE(identifier.hasStation());
        REQUIRE_FALSE(identifier.hasChannel());
    }

    SECTION("A rejected name preserves the previous one")
    {
        StreamIdentifier identifier;
        identifier.setNetwork("UU");
        identifier.setStation("CWU");
        identifier.setChannel("HHE");
        identifier.setLocationCode("01");
        REQUIRE_THROWS_AS(identifier.setNetwork("  "), std::invalid_argument);
        REQUIRE_THROWS_AS(identifier.setStation("  "), std::invalid_argument);
        REQUIRE_THROWS_AS(identifier.setChannel("  "), std::invalid_argument);
        REQUIRE(identifier.toString() == "UU.CWU.HHE.01");
    }

    SECTION("toString requires network, station, channel, and location code")
    {
        StreamIdentifier noNetwork;
        noNetwork.setStation("CWU");
        noNetwork.setChannel("HHE");
        noNetwork.setLocationCode("01");
        REQUIRE_THROWS_AS(noNetwork.toString(), std::runtime_error);

        StreamIdentifier noStation;
        noStation.setNetwork("UU");
        noStation.setChannel("HHE");
        noStation.setLocationCode("01");
        REQUIRE_THROWS_AS(noStation.toString(), std::runtime_error);

        StreamIdentifier noChannel;
        noChannel.setNetwork("UU");
        noChannel.setStation("CWU");
        noChannel.setLocationCode("01");
        REQUIRE_THROWS_AS(noChannel.toString(), std::runtime_error);

        StreamIdentifier noLocationCode;
        noLocationCode.setNetwork("UU");
        noLocationCode.setStation("CWU");
        noLocationCode.setChannel("HHE");
        REQUIRE_THROWS_AS(noLocationCode.toString(), std::runtime_error);
    }

    SECTION("Copy and move")
    {
        StreamIdentifier identifier;
        identifier.setNetwork("UU");
        identifier.setStation("CWU");
        identifier.setChannel("HHE");
        identifier.setLocationCode("01");

        // Copy constructor
        const StreamIdentifier copy{identifier};
        REQUIRE(copy.toString() == "UU.CWU.HHE.01");

        // Copy is deep: modifying the original doesn't touch the copy
        identifier.setNetwork("WY");
        identifier.setStation("YMR");
        identifier.setChannel("HHZ");
        identifier.setLocationCode("02");
        REQUIRE(copy.toString() == "UU.CWU.HHE.01");

        // Copy assignment
        StreamIdentifier copyAssigned;
        copyAssigned = copy;
        REQUIRE(copyAssigned.toString() == "UU.CWU.HHE.01");

        // Move constructor
        StreamIdentifier moved{std::move(copyAssigned)};
        REQUIRE(moved.toString() == "UU.CWU.HHE.01");

        // Move assignment
        StreamIdentifier moveAssigned;
        moveAssigned = std::move(moved);
        REQUIRE(moveAssigned.toString() == "UU.CWU.HHE.01");
    }
}
