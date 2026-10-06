#include <stdexcept>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/station_identifier.pb.h"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/stream_identifier.pb.h"

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

TEST_CASE("ULocalMagnitudeService::Corrections::StationIdentifier - protobuf",
          "[stationIdentifier]")
{
    namespace API = ULocalMagnitudeServiceAPI::V1::Magnitude;
    API::StationIdentifier message;
    message.set_network("UU");
    message.set_station("CWU");

    SECTION("From message")
    {
        const StationIdentifier identifier{message};
        REQUIRE(identifier.getNetwork() == "UU");
        REQUIRE(identifier.getStation() == "CWU");
        REQUIRE(identifier.toString() == "UU.CWU");
    }

    SECTION("From message is normalized like the setters")
    {
        // This is what makes "uu.ccut" from a client find UU.CCUT's
        // correction
        API::StationIdentifier messy;
        messy.set_network(" w y ");
        messy.set_station("\tymr\n");
        const StationIdentifier identifier{messy};
        REQUIRE(identifier.toString() == "WY.YMR");
    }

    SECTION("Network and station are required")
    {
        auto noNetwork = message;
        noNetwork.clear_network();
        REQUIRE_THROWS_AS(StationIdentifier {noNetwork},
                          std::invalid_argument);

        auto noStation = message;
        noStation.clear_station();
        REQUIRE_THROWS_AS(StationIdentifier {noStation},
                          std::invalid_argument);

        REQUIRE_THROWS_AS(StationIdentifier {API::StationIdentifier {}},
                          std::invalid_argument);
    }

    SECTION("Present but blank names are rejected")
    {
        auto blankNetwork = message;
        blankNetwork.set_network("");
        REQUIRE_THROWS_AS(StationIdentifier {blankNetwork},
                          std::invalid_argument);

        auto blankStation = message;
        blankStation.set_station(" \t ");
        REQUIRE_THROWS_AS(StationIdentifier {blankStation},
                          std::invalid_argument);
    }

    SECTION("To message")
    {
        StationIdentifier identifier;
        identifier.setNetwork("uu");
        identifier.setStation("ccut");
        const auto result = identifier.toMessage<API::StationIdentifier> ();
        REQUIRE(result.has_network());
        REQUIRE(result.has_station());
        REQUIRE(result.network() == "UU");
        REQUIRE(result.station() == "CCUT");
    }

    SECTION("To message requires network and station")
    {
        REQUIRE_THROWS_AS(
            StationIdentifier {}.toMessage<API::StationIdentifier> (),
            std::runtime_error);

        StationIdentifier noStation;
        noStation.setNetwork("UU");
        REQUIRE_THROWS_AS(noStation.toMessage<API::StationIdentifier> (),
                          std::runtime_error);

        StationIdentifier noNetwork;
        noNetwork.setStation("CWU");
        REQUIRE_THROWS_AS(noNetwork.toMessage<API::StationIdentifier> (),
                          std::runtime_error);
    }

    SECTION("Round trip through the wire format")
    {
        StationIdentifier identifier;
        identifier.setNetwork("WY");
        identifier.setStation("YMR");
        API::StationIdentifier parsed;
        REQUIRE(parsed.ParseFromString(
            identifier.toMessage<API::StationIdentifier> ()
           .SerializeAsString()));
        const StationIdentifier copy{parsed};
        REQUIRE(copy.toString() == "WY.YMR");
    }

    SECTION("A stream identifier message reaches the same station")
    {
        // The station magnitude request arrives as stream identifiers
        // while the corrections are keyed by station
        API::StreamIdentifier streamMessage;
        streamMessage.set_network("uu");
        streamMessage.set_station(" cwu");
        streamMessage.set_channel("HHE");
        streamMessage.set_location_code("01");
        const StationIdentifier fromStream{StreamIdentifier {streamMessage}};
        REQUIRE(fromStream.toString()
             == StationIdentifier {message}.toString());
        REQUIRE(fromStream.toMessage<API::StationIdentifier> ()
               .SerializeAsString()
             == StationIdentifier {message}.toMessage<API::StationIdentifier> ()
               .SerializeAsString());
    }
}
