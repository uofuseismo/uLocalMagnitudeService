#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitudeService/magnitude/observation.hpp"
#include "uLocalMagnitudeService/magnitude/amplitude.hpp"
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"

using namespace ULocalMagnitudeService::Magnitude;

namespace
{
Amplitude makeAmplitude(const std::string &network,
                        const std::string &station,
                        const std::string &channel,
                        const std::string &locationCode,
                        const double value)
{
    StreamIdentifier identifier;
    identifier.setNetwork(network);
    identifier.setStation(station);
    identifier.setChannel(channel);
    identifier.setLocationCode(locationCode);
    Amplitude amplitude;
    amplitude.setIdentifier(identifier);
    amplitude.setValue(value);
    return amplitude;
}

void checkAmplitudes(const Observation &observation)
{
    REQUIRE(observation.hasAmplitudes());
    REQUIRE(observation.getStationName() == "UU.CCUT");
    const auto [first, second] = observation.getAmplitudes();
    REQUIRE(first.getName() == "UU.CCUT.HHE.01");
    REQUIRE(second.getName() == "UU.CCUT.HHN.01");
    REQUIRE_THAT(first.getValue(),
                 Catch::Matchers::WithinAbs(0.3459206596016884, 1.e-14));
    REQUIRE_THAT(second.getValue(),
                 Catch::Matchers::WithinAbs(0.15187045093625784, 1.e-14));
}
}

TEST_CASE("ULocalMagnitudeService::Magnitude::Observation", "[observation]")
{
    // From testing/data/amp-ml-uu80157466.csv
    const auto east
        = makeAmplitude("UU", "CCUT", "HHE", "01", 0.3459206596016884);
    const auto north
        = makeAmplitude("UU", "CCUT", "HHN", "01", 0.15187045093625784);
    constexpr double distance{66775.14936328208};
    constexpr double depth{5140};

    SECTION("Defaults")
    {
        const Observation observation;
        REQUIRE_FALSE(observation.hasAmplitudes());
        REQUIRE_FALSE(observation.hasEpicentralDistance());
        REQUIRE_FALSE(observation.hasDepth());
        REQUIRE_THROWS_AS(observation.getAmplitudes(), std::runtime_error);
        REQUIRE_THROWS_AS(observation.getAmplitudesReference(),
                          std::runtime_error);
        REQUIRE_THROWS_AS(observation.getStationName(), std::runtime_error);
        REQUIRE_THROWS_AS(observation.getEpicentralDistance(),
                          std::runtime_error);
        REQUIRE_THROWS_AS(observation.getDepth(), std::runtime_error);
    }

    SECTION("Amplitudes")
    {
        Observation observation;
        observation.setAmplitudes(std::pair {east, north});
        checkAmplitudes(observation);
    }

    SECTION("Amplitudes by reference")
    {
        Observation observation;
        observation.setAmplitudes(std::pair {east, north});
        const auto &[first, second] = observation.getAmplitudesReference();
        REQUIRE(first.getName() == "UU.CCUT.HHE.01");
        REQUIRE(second.getName() == "UU.CCUT.HHN.01");
        REQUIRE(first.getValue() == east.getValue());
        REQUIRE(second.getValue() == north.getValue());
    }

    SECTION("Amplitudes by move")
    {
        Observation observation;
        auto amplitudes = std::pair {east, north};
        observation.setAmplitudes(std::move(amplitudes));
        checkAmplitudes(observation);
    }

    SECTION("Amplitudes are kept in the order given")
    {
        Observation observation;
        observation.setAmplitudes(std::pair {north, east});
        const auto [first, second] = observation.getAmplitudes();
        REQUIRE(first.getName() == "UU.CCUT.HHN.01");
        REQUIRE(second.getName() == "UU.CCUT.HHE.01");
    }

    SECTION("Other horizontal channel conventions")
    {
        Observation observation;
        // BOZ in Yellowstone uses 1/2 rather than E/N
        observation.setAmplitudes(
            std::pair {makeAmplitude("US", "BOZ", "BH1", "00", 0.126),
                       makeAmplitude("US", "BOZ", "BH2", "00", 0.114)});
        REQUIRE(observation.getStationName() == "US.BOZ");
        // A blank location code
        observation.setAmplitudes(
            std::pair {makeAmplitude("WY", "YFT", "EHE", "", 0.17),
                       makeAmplitude("WY", "YFT", "EHN", "  ", 0.19)});
        REQUIRE(observation.getStationName() == "WY.YFT");
    }

    SECTION("Amplitudes need values and identifiers")
    {
        Observation observation;
        Amplitude noValue;
        noValue.setIdentifier(north.getIdentifier());
        REQUIRE_THROWS_AS(observation.setAmplitudes(std::pair {east, noValue}),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(observation.setAmplitudes(std::pair {noValue, east}),
                          std::invalid_argument);

        Amplitude noIdentifier;
        noIdentifier.setValue(0.15187045093625784);
        REQUIRE_THROWS_AS(
            observation.setAmplitudes(std::pair {east, noIdentifier}),
            std::invalid_argument);
        REQUIRE_THROWS_AS(
            observation.setAmplitudes(std::pair {noIdentifier, east}),
            std::invalid_argument);
        REQUIRE_FALSE(observation.hasAmplitudes());
    }

    SECTION("Mismatched amplitudes are rejected")
    {
        Observation observation;
        // Same channel
        REQUIRE_THROWS_AS(observation.setAmplitudes(std::pair {east, east}),
                          std::invalid_argument);
        // Different network
        REQUIRE_THROWS_AS(
            observation.setAmplitudes(
                std::pair {east, makeAmplitude("WY", "CCUT", "HHN", "01", 1)}),
            std::invalid_argument);
        // Different station
        REQUIRE_THROWS_AS(
            observation.setAmplitudes(
                std::pair {east, makeAmplitude("UU", "LCMT", "HHN", "01", 1)}),
            std::invalid_argument);
        // Different location code - US.DUG.HH1.00 and US.DUG.HH2.02
        REQUIRE_THROWS_AS(
            observation.setAmplitudes(
                std::pair {makeAmplitude("US", "DUG", "HH1", "00", 1),
                           makeAmplitude("US", "DUG", "HH2", "02", 1)}),
            std::invalid_argument);
        // Different band code - UU.CWU.HHE.01 and UU.CWU.ENN.01
        REQUIRE_THROWS_AS(
            observation.setAmplitudes(
                std::pair {makeAmplitude("UU", "CWU", "HHE", "01", 1),
                           makeAmplitude("UU", "CWU", "ENN", "01", 1)}),
            std::invalid_argument);
        // Different instrument code
        REQUIRE_THROWS_AS(
            observation.setAmplitudes(
                std::pair {east, makeAmplitude("UU", "CCUT", "HNN", "01", 1)}),
            std::invalid_argument);
        REQUIRE_FALSE(observation.hasAmplitudes());
    }

    SECTION("A rejected pair preserves the previous one")
    {
        Observation observation;
        observation.setAmplitudes(std::pair {east, north});
        REQUIRE_THROWS_AS(observation.setAmplitudes(std::pair {east, east}),
                          std::invalid_argument);
        checkAmplitudes(observation);
    }

    SECTION("Setting again overwrites")
    {
        Observation observation;
        observation.setAmplitudes(std::pair {east, north});
        observation.setAmplitudes(
            std::pair {makeAmplitude("UU", "LCMT", "HHE", "01", 2.7),
                       makeAmplitude("UU", "LCMT", "HHN", "01", 3.3)});
        REQUIRE(observation.getStationName() == "UU.LCMT");
    }

    SECTION("Epicentral distance")
    {
        Observation observation;
        observation.setEpicentralDistance(distance);
        REQUIRE(observation.hasEpicentralDistance());
        REQUIRE_THAT(observation.getEpicentralDistance(),
                     Catch::Matchers::WithinAbs(distance, 1.e-8));
        // Limits are inclusive
        observation.setEpicentralDistance(0);
        REQUIRE(observation.getEpicentralDistance() == 0);
        observation.setEpicentralDistance(21000000);
        REQUIRE(observation.getEpicentralDistance() == 21000000);
        // Out of range is rejected and the previous value is kept
        REQUIRE_THROWS_AS(observation.setEpicentralDistance(-1),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(observation.setEpicentralDistance(21000001),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(observation.setEpicentralDistance(
                              std::numeric_limits<double>::quiet_NaN()),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(observation.setEpicentralDistance(
                              std::numeric_limits<double>::infinity()),
                          std::invalid_argument);
        REQUIRE(observation.getEpicentralDistance() == 21000000);
    }

    SECTION("Depth")
    {
        Observation observation;
        observation.setDepth(depth);
        REQUIRE(observation.hasDepth());
        REQUIRE(observation.getDepth() == depth);
        // Above the datum is fine
        observation.setDepth(-2000);
        REQUIRE(observation.getDepth() == -2000);
        // Limits are inclusive
        observation.setDepth(-8600);
        REQUIRE(observation.getDepth() == -8600);
        observation.setDepth(900000);
        REQUIRE(observation.getDepth() == 900000);
        // Out of range is rejected and the previous value is kept
        REQUIRE_THROWS_AS(observation.setDepth(-8601), std::invalid_argument);
        REQUIRE_THROWS_AS(observation.setDepth(900001), std::invalid_argument);
        REQUIRE_THROWS_AS(
            observation.setDepth(std::numeric_limits<double>::quiet_NaN()),
            std::invalid_argument);
        REQUIRE_THROWS_AS(
            observation.setDepth(std::numeric_limits<double>::infinity()),
            std::invalid_argument);
        REQUIRE(observation.getDepth() == 900000);
    }

    SECTION("Copy and move")
    {
        Observation observation;
        observation.setAmplitudes(std::pair {east, north});
        observation.setEpicentralDistance(distance);
        observation.setDepth(depth);
        const auto check = [&](const Observation &other)
        {
            checkAmplitudes(other);
            REQUIRE_THAT(other.getEpicentralDistance(),
                         Catch::Matchers::WithinAbs(distance, 1.e-8));
            REQUIRE(other.getDepth() == depth);
        };

        // Copy constructor
        const Observation copy{observation};
        check(copy);

        // Copy is deep: modifying the original doesn't touch the copy
        observation.setAmplitudes(
            std::pair {makeAmplitude("UU", "LCMT", "HHE", "01", 2.7),
                       makeAmplitude("UU", "LCMT", "HHN", "01", 3.3)});
        observation.setEpicentralDistance(1000);
        observation.setDepth(0);
        check(copy);

        // Copy assignment
        Observation copyAssigned;
        copyAssigned = copy;
        check(copyAssigned);

        // Move constructor
        Observation moved{std::move(copyAssigned)};
        check(moved);

        // Move assignment
        Observation moveAssigned;
        moveAssigned = std::move(moved);
        check(moveAssigned);
    }
}
