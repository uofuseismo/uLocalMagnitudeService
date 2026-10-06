#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitudeService/magnitude/amplitude.hpp"
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/amplitude.pb.h"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/stream_identifier.pb.h"

using namespace ULocalMagnitudeService::Magnitude;

TEST_CASE("ULocalMagnitudeService::Magnitude::Amplitude", "[amplitude]")
{
    StreamIdentifier identifier;
    identifier.setNetwork("UU");
    identifier.setStation("CWU");
    identifier.setChannel("HHE");
    identifier.setLocationCode("01");
    constexpr double value{0.35};

    SECTION("Defaults")
    {
        const Amplitude amplitude;
        REQUIRE_FALSE(amplitude.hasValue());
        REQUIRE_FALSE(amplitude.hasIdentifier());
        REQUIRE_THROWS_AS(amplitude.getValue(), std::runtime_error);
        REQUIRE_THROWS_AS(amplitude.getIdentifier(), std::runtime_error);
        REQUIRE_THROWS_AS(amplitude.getName(), std::runtime_error);
    }

    SECTION("Value")
    {
        Amplitude amplitude;
        amplitude.setValue(value);
        REQUIRE(amplitude.hasValue());
        REQUIRE_THAT(amplitude.getValue(),
                     Catch::Matchers::WithinAbs(value, 1.e-14));
        // Setting again overwrites
        amplitude.setValue(2.5);
        REQUIRE_THAT(amplitude.getValue(),
                     Catch::Matchers::WithinAbs(2.5, 1.e-14));
        // Tiny but positive is fine
        amplitude.setValue(std::numeric_limits<double>::min());
        REQUIRE(amplitude.hasValue());
    }

    SECTION("Non-positive values are rejected")
    {
        Amplitude amplitude;
        REQUIRE_THROWS_AS(amplitude.setValue(0), std::invalid_argument);
        REQUIRE_THROWS_AS(amplitude.setValue(-1), std::invalid_argument);
        REQUIRE_FALSE(amplitude.hasValue());
        // A rejected value preserves the previous one
        amplitude.setValue(value);
        REQUIRE_THROWS_AS(amplitude.setValue(-value), std::invalid_argument);
        REQUIRE_THAT(amplitude.getValue(),
                     Catch::Matchers::WithinAbs(value, 1.e-14));
    }

    SECTION("Non-finite values are rejected")
    {
        // NaN <= 0 is false so NaN must be caught explicitly
        Amplitude amplitude;
        REQUIRE_THROWS_AS(
            amplitude.setValue(std::numeric_limits<double>::quiet_NaN()),
            std::invalid_argument);
        REQUIRE_THROWS_AS(
            amplitude.setValue(std::numeric_limits<double>::infinity()),
            std::invalid_argument);
        REQUIRE_THROWS_AS(
            amplitude.setValue(-std::numeric_limits<double>::infinity()),
            std::invalid_argument);
        REQUIRE_FALSE(amplitude.hasValue());
        // Huge but finite is fine
        REQUIRE_NOTHROW(
            amplitude.setValue(std::numeric_limits<double>::max()));
    }

    SECTION("Identifier")
    {
        Amplitude amplitude;
        amplitude.setIdentifier(identifier);
        REQUIRE(amplitude.hasIdentifier());
        REQUIRE(amplitude.getName() == "UU.CWU.HHE.01");
        const auto identifierBack = amplitude.getIdentifier();
        REQUIRE(identifierBack.getNetwork() == "UU");
        REQUIRE(identifierBack.getStation() == "CWU");
        REQUIRE(identifierBack.getChannel() == "HHE");
        REQUIRE(identifierBack.getLocationCode() == "01");
        // The identifier is copied in
        identifier.setStation("CTU");
        REQUIRE(amplitude.getName() == "UU.CWU.HHE.01");
    }

    SECTION("Move identifier")
    {
        Amplitude amplitude;
        auto identifierCopy = identifier;
        amplitude.setIdentifier(std::move(identifierCopy));
        REQUIRE(amplitude.hasIdentifier());
        REQUIRE(amplitude.getName() == "UU.CWU.HHE.01");

        // Incomplete identifiers are rejected
        StreamIdentifier noLocationCode;
        noLocationCode.setNetwork("WY");
        noLocationCode.setStation("YMR");
        noLocationCode.setChannel("HHZ");
        REQUIRE_THROWS_AS(amplitude.setIdentifier(std::move(noLocationCode)),
                          std::invalid_argument);
        REQUIRE(amplitude.getName() == "UU.CWU.HHE.01");
    }

    SECTION("Blank location code")
    {
        identifier.setLocationCode("  ");
        Amplitude amplitude;
        amplitude.setIdentifier(identifier);
        REQUIRE(amplitude.getName() == "UU.CWU.HHE");
    }

    SECTION("Incomplete identifiers are rejected")
    {
        Amplitude amplitude;

        StreamIdentifier noNetwork;
        noNetwork.setStation("CWU");
        noNetwork.setChannel("HHE");
        noNetwork.setLocationCode("01");
        REQUIRE_THROWS_AS(amplitude.setIdentifier(noNetwork),
                          std::invalid_argument);

        StreamIdentifier noStation;
        noStation.setNetwork("UU");
        noStation.setChannel("HHE");
        noStation.setLocationCode("01");
        REQUIRE_THROWS_AS(amplitude.setIdentifier(noStation),
                          std::invalid_argument);

        StreamIdentifier noChannel;
        noChannel.setNetwork("UU");
        noChannel.setStation("CWU");
        noChannel.setLocationCode("01");
        REQUIRE_THROWS_AS(amplitude.setIdentifier(noChannel),
                          std::invalid_argument);

        StreamIdentifier noLocationCode;
        noLocationCode.setNetwork("UU");
        noLocationCode.setStation("CWU");
        noLocationCode.setChannel("HHE");
        REQUIRE_THROWS_AS(amplitude.setIdentifier(noLocationCode),
                          std::invalid_argument);

        REQUIRE_FALSE(amplitude.hasIdentifier());

        // A rejected identifier preserves the previous one
        amplitude.setIdentifier(identifier);
        REQUIRE_THROWS_AS(amplitude.setIdentifier(noChannel),
                          std::invalid_argument);
        REQUIRE(amplitude.getName() == "UU.CWU.HHE.01");
    }

    SECTION("Copy and move")
    {
        Amplitude amplitude;
        amplitude.setValue(value);
        amplitude.setIdentifier(identifier);

        // Copy constructor
        const Amplitude copy{amplitude};
        REQUIRE(copy.getName() == "UU.CWU.HHE.01");
        REQUIRE_THAT(copy.getValue(),
                     Catch::Matchers::WithinAbs(value, 1.e-14));

        // Copy is deep: modifying the original doesn't touch the copy
        StreamIdentifier otherIdentifier;
        otherIdentifier.setNetwork("WY");
        otherIdentifier.setStation("YMR");
        otherIdentifier.setChannel("HHZ");
        otherIdentifier.setLocationCode("02");
        amplitude.setIdentifier(otherIdentifier);
        amplitude.setValue(2 * value);
        REQUIRE(copy.getName() == "UU.CWU.HHE.01");
        REQUIRE_THAT(copy.getValue(),
                     Catch::Matchers::WithinAbs(value, 1.e-14));

        // Copy assignment
        Amplitude copyAssigned;
        copyAssigned = copy;
        REQUIRE(copyAssigned.getName() == "UU.CWU.HHE.01");

        // Move constructor
        Amplitude moved{std::move(copyAssigned)};
        REQUIRE(moved.getName() == "UU.CWU.HHE.01");

        // Move assignment
        Amplitude moveAssigned;
        moveAssigned = std::move(moved);
        REQUIRE(moveAssigned.getName() == "UU.CWU.HHE.01");
        REQUIRE_THAT(moveAssigned.getValue(),
                     Catch::Matchers::WithinAbs(value, 1.e-14));
    }
}

TEST_CASE("ULocalMagnitudeService::Magnitude::Amplitude - protobuf",
          "[amplitude]")
{
    namespace API = ULocalMagnitudeServiceAPI::V1::Magnitude;
    API::StreamIdentifier identifierMessage;
    identifierMessage.set_network("UU");
    identifierMessage.set_station("CCUT");
    identifierMessage.set_channel("HHE");
    identifierMessage.set_location_code("01");
    // UU.CCUT.HHE for evid 80157466 - AQMS stores this in cm
    constexpr double valueInCentimeters{0.03459206596016884};

    API::Amplitude message;
    *message.mutable_stream_identifier() = identifierMessage;
    message.set_value(10*valueInCentimeters);
    message.set_units(API::Amplitude_Units_MILLIMETERS);

    SECTION("From message")
    {
        const Amplitude amplitude{message};
        REQUIRE(amplitude.getName() == "UU.CCUT.HHE.01");
        REQUIRE_THAT(amplitude.getValue(),
                     Catch::Matchers::WithinRel(10*valueInCentimeters,
                                                1.e-14));
    }

    SECTION("Units are converted to millimeters")
    {
        message.set_value(valueInCentimeters);
        message.set_units(API::Amplitude_Units_CENTIMETERS);
        REQUIRE_THAT(Amplitude {message}.getValue(),
                     Catch::Matchers::WithinRel(10*valueInCentimeters,
                                                1.e-14));

        message.set_value(valueInCentimeters/100);
        message.set_units(API::Amplitude_Units_METERS);
        REQUIRE_THAT(Amplitude {message}.getValue(),
                     Catch::Matchers::WithinRel(10*valueInCentimeters,
                                                1.e-14));
    }

    SECTION("Units are required")
    {
        auto noUnits = message;
        noUnits.clear_units();
        REQUIRE_THROWS_AS(Amplitude {noUnits}, std::invalid_argument);

        auto unknownUnits = message;
        unknownUnits.set_units(API::Amplitude_Units_UNKNOWN);
        REQUIRE_THROWS_AS(Amplitude {unknownUnits}, std::invalid_argument);
    }

    SECTION("Units the server doesn't know about are rejected")
    {
        // The enum is open so a newer client can send a value this server
        // was never built with
        auto futureUnits = message;
        futureUnits.set_units(static_cast<API::Amplitude_Units> (99));
        REQUIRE_THROWS_AS(Amplitude {futureUnits}, std::invalid_argument);
    }

    SECTION("Value is required and must be positive and finite")
    {
        auto noValue = message;
        noValue.clear_value();
        REQUIRE_THROWS_AS(Amplitude {noValue}, std::invalid_argument);

        for (const double value : {0.0, -0.35,
                                   std::numeric_limits<double>::quiet_NaN(),
                                   std::numeric_limits<double>::infinity()})
        {
            INFO("Value: " << value);
            auto badValue = message;
            badValue.set_value(value);
            REQUIRE_THROWS_AS(Amplitude {badValue}, std::invalid_argument);
        }
    }

    SECTION("Conversion can't overflow a finite value")
    {
        // Finite in meters but not in millimeters
        auto huge = message;
        huge.set_value(std::numeric_limits<double>::max());
        huge.set_units(API::Amplitude_Units_METERS);
        REQUIRE_THROWS_AS(Amplitude {huge}, std::invalid_argument);
    }

    SECTION("A valid stream identifier is required")
    {
        auto noIdentifier = message;
        noIdentifier.clear_stream_identifier();
        REQUIRE_THROWS_AS(Amplitude {noIdentifier}, std::invalid_argument);

        auto noStation = message;
        noStation.mutable_stream_identifier()->clear_station();
        REQUIRE_THROWS_AS(Amplitude {noStation}, std::invalid_argument);
    }

    SECTION("To message")
    {
        StreamIdentifier identifier;
        identifier.setNetwork("UU");
        identifier.setStation("CCUT");
        identifier.setChannel("HHN");
        identifier.setLocationCode("01");
        Amplitude amplitude;
        amplitude.setIdentifier(identifier);
        amplitude.setValue(0.15187045093625784);
        const auto result = amplitude.toMessage<API::Amplitude> ();
        // Always written in millimeters
        REQUIRE(result.units() == API::Amplitude_Units_MILLIMETERS);
        REQUIRE(result.value() == 0.15187045093625784);
        REQUIRE(result.stream_identifier().network() == "UU");
        REQUIRE(result.stream_identifier().station() == "CCUT");
        REQUIRE(result.stream_identifier().channel() == "HHN");
        REQUIRE(result.stream_identifier().location_code() == "01");
    }

    SECTION("To message requires a value and an identifier")
    {
        REQUIRE_THROWS_AS(Amplitude {}.toMessage<API::Amplitude> (),
                          std::runtime_error);

        Amplitude noIdentifier;
        noIdentifier.setValue(0.35);
        REQUIRE_THROWS_AS(noIdentifier.toMessage<API::Amplitude> (),
                          std::runtime_error);

        StreamIdentifier identifier;
        identifier.setNetwork("UU");
        identifier.setStation("CCUT");
        identifier.setChannel("HHN");
        identifier.setLocationCode("01");
        Amplitude noValue;
        noValue.setIdentifier(identifier);
        REQUIRE_THROWS_AS(noValue.toMessage<API::Amplitude> (),
                          std::runtime_error);
    }

    SECTION("Round trip through the wire format")
    {
        // Sent in cm, comes back out in mm
        message.set_value(valueInCentimeters);
        message.set_units(API::Amplitude_Units_CENTIMETERS);
        API::Amplitude received;
        REQUIRE(received.ParseFromString(message.SerializeAsString()));
        const Amplitude amplitude{received};

        API::Amplitude echoed;
        REQUIRE(echoed.ParseFromString(
            amplitude.toMessage<API::Amplitude> ().SerializeAsString()));
        REQUIRE(echoed.units() == API::Amplitude_Units_MILLIMETERS);
        REQUIRE_THAT(echoed.value(),
                     Catch::Matchers::WithinRel(10*valueInCentimeters,
                                                1.e-14));
        const Amplitude again{echoed};
        REQUIRE(again.getName() == amplitude.getName());
        REQUIRE(again.getValue() == amplitude.getValue());
    }
}
