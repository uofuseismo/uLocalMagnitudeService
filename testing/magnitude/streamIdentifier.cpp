#include <stdexcept>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/stream_identifier.pb.h"

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

TEST_CASE("ULocalMagnitudeService::Magnitude::StreamIdentifier - protobuf",
          "[streamIdentifier]")
{
    namespace API = ULocalMagnitudeServiceAPI::V1::Magnitude;
    API::StreamIdentifier message;
    message.set_network("UU");
    message.set_station("CWU");
    message.set_channel("HHE");
    message.set_location_code("01");

    SECTION("From message")
    {
        const StreamIdentifier identifier{message};
        REQUIRE(identifier.getNetwork() == "UU");
        REQUIRE(identifier.getStation() == "CWU");
        REQUIRE(identifier.getChannel() == "HHE");
        REQUIRE(identifier.getLocationCode() == "01");
        REQUIRE(identifier.toString() == "UU.CWU.HHE.01");
    }

    SECTION("From message is normalized like the setters")
    {
        API::StreamIdentifier messy;
        messy.set_network(" w y ");
        messy.set_station("\tymr\n");
        messy.set_channel(" e h z");
        messy.set_location_code("  ");
        const StreamIdentifier identifier{messy};
        REQUIRE(identifier.toString() == "WY.YMR.EHZ");
        REQUIRE(identifier.hasLocationCode());
        REQUIRE(identifier.getLocationCode().empty());
    }

    SECTION("A missing location code is blank")
    {
        message.clear_location_code();
        const StreamIdentifier identifier{message};
        REQUIRE(identifier.hasLocationCode());
        REQUIRE(identifier.getLocationCode().empty());
        REQUIRE(identifier.toString() == "UU.CWU.HHE");
    }

    SECTION("Network, station, and channel are required")
    {
        auto noNetwork = message;
        noNetwork.clear_network();
        REQUIRE_THROWS_AS(StreamIdentifier {noNetwork}, std::invalid_argument);

        auto noStation = message;
        noStation.clear_station();
        REQUIRE_THROWS_AS(StreamIdentifier {noStation}, std::invalid_argument);

        auto noChannel = message;
        noChannel.clear_channel();
        REQUIRE_THROWS_AS(StreamIdentifier {noChannel}, std::invalid_argument);
    }

    SECTION("Present but blank names are rejected")
    {
        auto blankNetwork = message;
        blankNetwork.set_network("");
        REQUIRE_THROWS_AS(StreamIdentifier {blankNetwork},
                          std::invalid_argument);

        auto blankStation = message;
        blankStation.set_station("  ");
        REQUIRE_THROWS_AS(StreamIdentifier {blankStation},
                          std::invalid_argument);

        auto blankChannel = message;
        blankChannel.set_channel("\t");
        REQUIRE_THROWS_AS(StreamIdentifier {blankChannel},
                          std::invalid_argument);
    }

    SECTION("To message")
    {
        StreamIdentifier identifier;
        identifier.setNetwork("uu");
        identifier.setStation("cwu");
        identifier.setChannel("hhe");
        identifier.setLocationCode("01");
        const auto result = identifier.toMessage<API::StreamIdentifier> ();
        REQUIRE(result.network() == "UU");
        REQUIRE(result.station() == "CWU");
        REQUIRE(result.channel() == "HHE");
        REQUIRE(result.location_code() == "01");
    }

    SECTION("To message keeps a blank location code")
    {
        StreamIdentifier identifier;
        identifier.setNetwork("NN");
        identifier.setStation("PRN");
        identifier.setChannel("HHN");
        identifier.setLocationCode("");
        const auto result = identifier.toMessage<API::StreamIdentifier> ();
        REQUIRE(result.has_location_code());
        REQUIRE(result.location_code().empty());
    }

    SECTION("To message requires a complete identifier")
    {
        REQUIRE_THROWS_AS(
            StreamIdentifier {}.toMessage<API::StreamIdentifier> (),
            std::runtime_error);
        StreamIdentifier noLocationCode;
        noLocationCode.setNetwork("UU");
        noLocationCode.setStation("CWU");
        noLocationCode.setChannel("HHE");
        REQUIRE_THROWS_AS(noLocationCode.toMessage<API::StreamIdentifier> (),
                          std::runtime_error);
    }

    SECTION("Round trip through the wire format")
    {
        for (const auto &locationCode : {"01", ""})
        {
            INFO("Location code: '" << locationCode << "'");
            StreamIdentifier identifier;
            identifier.setNetwork("WY");
            identifier.setStation("YMR");
            identifier.setChannel("HHZ");
            identifier.setLocationCode(locationCode);
            const auto bytes
                = identifier.toMessage<API::StreamIdentifier> ()
                 .SerializeAsString();
            API::StreamIdentifier parsed;
            REQUIRE(parsed.ParseFromString(bytes));
            const StreamIdentifier copy{parsed};
            REQUIRE(copy.toString() == identifier.toString());
            REQUIRE(copy.getLocationCode() == identifier.getLocationCode());
        }
    }
}
