#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitudeService/magnitude/amplitude.hpp"
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"

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
