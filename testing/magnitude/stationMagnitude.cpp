#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include "uLocalMagnitudeService/magnitude/stationMagnitude.hpp"
#include "uLocalMagnitudeService/magnitude/amplitude.hpp"
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"

using namespace ULocalMagnitudeService::Magnitude;

namespace
{
constexpr double nan{std::numeric_limits<double>::quiet_NaN()};
constexpr double infinity{std::numeric_limits<double>::infinity()};

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
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

/// UU.CCUT for evid 80157466 in mm.
std::pair<Amplitude, Amplitude> ccutAmplitudes()
{
    return std::pair {makeAmplitude("UU", "CCUT", "HHE", "01",
                                    0.3459206596016884),
                      makeAmplitude("UU", "CCUT", "HHN", "01",
                                    0.15187045093625784)};
}

StationMagnitude makeMagnitude()
{
    StationMagnitude magnitude;
    magnitude.setAmplitudes(ccutAmplitudes());
    magnitude.setValue(2.204987145462916);
    magnitude.setStationCorrection(0.31);
    magnitude.setDistanceCorrection(2.8);
    return magnitude;
}

void checkMagnitude(const StationMagnitude &magnitude)
{
    REQUIRE(magnitude.getStationName() == "UU.CCUT");
    const auto [first, second] = magnitude.getAmplitudes();
    REQUIRE(first.getName() == "UU.CCUT.HHE.01");
    REQUIRE(second.getName() == "UU.CCUT.HHN.01");
    REQUIRE(first.getValue() == 0.3459206596016884);
    REQUIRE(second.getValue() == 0.15187045093625784);
    REQUIRE(magnitude.getValue() == 2.204987145462916);
    REQUIRE(magnitude.getStationCorrection() == 0.31);
    REQUIRE(magnitude.getDistanceCorrection() == 2.8);
}
}

TEST_CASE("ULocalMagnitudeService::Magnitude::StationMagnitude",
          "[stationMagnitude]")
{
    SECTION("Defaults")
    {
        const StationMagnitude magnitude;
        REQUIRE_FALSE(magnitude.hasValue());
        REQUIRE_FALSE(magnitude.hasStationCorrection());
        REQUIRE_FALSE(magnitude.hasDistanceCorrection());
        REQUIRE_FALSE(magnitude.hasAmplitudes());
        REQUIRE_THROWS_AS(magnitude.getValue(), std::runtime_error);
        REQUIRE_THROWS_AS(magnitude.getStationCorrection(),
                          std::runtime_error);
        REQUIRE_THROWS_AS(magnitude.getDistanceCorrection(),
                          std::runtime_error);
        REQUIRE_THROWS_AS(magnitude.getAmplitudes(), std::runtime_error);
        REQUIRE_THROWS_AS(magnitude.getStationName(), std::runtime_error);
    }

    SECTION("Setters and getters")
    {
        const auto magnitude = makeMagnitude();
        REQUIRE(magnitude.hasValue());
        REQUIRE(magnitude.hasStationCorrection());
        REQUIRE(magnitude.hasDistanceCorrection());
        REQUIRE(magnitude.hasAmplitudes());
        checkMagnitude(magnitude);
    }

    SECTION("The station name comes from the amplitudes")
    {
        StationMagnitude magnitude;
        magnitude.setAmplitudes(
            std::pair {makeAmplitude("us", " dug", "HH1", "00", 0.14),
                       makeAmplitude("US", "DUG", "HH2", "00", 0.15)});
        REQUIRE(magnitude.getStationName() == "US.DUG");
        // A blank location code doesn't leak into the name
        magnitude.setAmplitudes(
            std::pair {makeAmplitude("WY", "YMR", "HHE", "", 0.7),
                       makeAmplitude("WY", "YMR", "HHN", "  ", 0.8)});
        REQUIRE(magnitude.getStationName() == "WY.YMR");
    }

    SECTION("Amplitudes are kept in the order given")
    {
        auto [east, north] = ccutAmplitudes();
        StationMagnitude magnitude;
        magnitude.setAmplitudes(std::pair {north, east});
        REQUIRE(magnitude.getAmplitudes().first.getName() == "UU.CCUT.HHN.01");
        REQUIRE(magnitude.getAmplitudes().second.getName()
             == "UU.CCUT.HHE.01");
    }

    SECTION("Amplitudes need values and identifiers")
    {
        const auto [east, north] = ccutAmplitudes();
        Amplitude noValue;
        noValue.setIdentifier(north.getIdentifier());
        Amplitude noIdentifier;
        noIdentifier.setValue(0.15);

        StationMagnitude magnitude;
        REQUIRE_THROWS_AS(magnitude.setAmplitudes(std::pair {east, noValue}),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(magnitude.setAmplitudes(std::pair {noValue, east}),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(
            magnitude.setAmplitudes(std::pair {east, noIdentifier}),
            std::invalid_argument);
        REQUIRE_THROWS_AS(
            magnitude.setAmplitudes(std::pair {noIdentifier, east}),
            std::invalid_argument);
        REQUIRE_FALSE(magnitude.hasAmplitudes());
    }

    SECTION("Mismatched amplitudes are rejected")
    {
        const auto [east, north] = ccutAmplitudes();
        StationMagnitude magnitude;
        // Same channel twice
        REQUIRE_THROWS_AS(magnitude.setAmplitudes(std::pair {east, east}),
                          std::invalid_argument);
        // Different stations
        REQUIRE_THROWS_AS(
            magnitude.setAmplitudes(
                std::pair {east,
                           makeAmplitude("UU", "LCMT", "HHN", "01", 0.33)}),
            std::invalid_argument);
        // Same station name on another network
        REQUIRE_THROWS_AS(
            magnitude.setAmplitudes(
                std::pair {east,
                           makeAmplitude("WY", "CCUT", "HHN", "01", 0.15)}),
            std::invalid_argument);
        // Different location codes
        REQUIRE_THROWS_AS(
            magnitude.setAmplitudes(
                std::pair {east,
                           makeAmplitude("UU", "CCUT", "HHN", "02", 0.15)}),
            std::invalid_argument);
        // Different sensors
        REQUIRE_THROWS_AS(
            magnitude.setAmplitudes(
                std::pair {east,
                           makeAmplitude("UU", "CCUT", "ENN", "01", 0.15)}),
            std::invalid_argument);
        REQUIRE_FALSE(magnitude.hasAmplitudes());
    }

    SECTION("A rejected amplitude pair preserves the previous one")
    {
        auto magnitude = makeMagnitude();
        const auto [east, north] = ccutAmplitudes();
        REQUIRE_THROWS_AS(
            magnitude.setAmplitudes(
                std::pair {east,
                           makeAmplitude("UU", "LCMT", "HHN", "01", 0.33)}),
            std::invalid_argument);
        checkMagnitude(magnitude);
    }

    SECTION("Negative and zero values are fine")
    {
        // Small events have negative magnitudes and corrections can be
        // negative or zero
        StationMagnitude magnitude;
        magnitude.setValue(-0.5);
        REQUIRE(magnitude.getValue() == -0.5);
        magnitude.setValue(0);
        REQUIRE(magnitude.getValue() == 0);
        magnitude.setStationCorrection(-0.32);
        REQUIRE(magnitude.getStationCorrection() == -0.32);
        magnitude.setStationCorrection(0);
        REQUIRE(magnitude.getStationCorrection() == 0);
        magnitude.setDistanceCorrection(0);
        REQUIRE(magnitude.getDistanceCorrection() == 0);
    }

    SECTION("Non-finite values are rejected")
    {
        auto magnitude = makeMagnitude();
        for (const auto bad : {nan, infinity, -infinity})
        {
            REQUIRE_THROWS_AS(magnitude.setValue(bad), std::invalid_argument);
            REQUIRE_THROWS_AS(magnitude.setStationCorrection(bad),
                              std::invalid_argument);
            REQUIRE_THROWS_AS(magnitude.setDistanceCorrection(bad),
                              std::invalid_argument);
        }
        // The previous values are kept
        checkMagnitude(magnitude);
    }

    SECTION("Setting again overwrites")
    {
        auto magnitude = makeMagnitude();
        magnitude.setAmplitudes(
            std::pair {makeAmplitude("UU", "LCMT", "HHE", "01", 2.7),
                       makeAmplitude("UU", "LCMT", "HHN", "01", 3.3)});
        magnitude.setValue(2.158753817909035);
        magnitude.setStationCorrection(-0.12);
        magnitude.setDistanceCorrection(2.1);
        REQUIRE(magnitude.getStationName() == "UU.LCMT");
        REQUIRE(magnitude.getAmplitudes().first.getName() == "UU.LCMT.HHE.01");
        REQUIRE(magnitude.getValue() == 2.158753817909035);
        REQUIRE(magnitude.getStationCorrection() == -0.12);
        REQUIRE(magnitude.getDistanceCorrection() == 2.1);
    }

    SECTION("Copy and move")
    {
        auto magnitude = makeMagnitude();

        // Copy constructor
        const StationMagnitude copy{magnitude};
        checkMagnitude(copy);

        // Copy is deep: modifying the original doesn't touch the copy
        magnitude.setAmplitudes(
            std::pair {makeAmplitude("UU", "LCMT", "HHE", "01", 2.7),
                       makeAmplitude("UU", "LCMT", "HHN", "01", 3.3)});
        magnitude.setValue(1);
        magnitude.setStationCorrection(1);
        magnitude.setDistanceCorrection(1);
        checkMagnitude(copy);

        // Copy assignment
        StationMagnitude copyAssigned;
        copyAssigned = copy;
        checkMagnitude(copyAssigned);

        // Move constructor
        StationMagnitude moved{std::move(copyAssigned)};
        checkMagnitude(moved);

        // Move assignment
        StationMagnitude moveAssigned;
        moveAssigned = std::move(moved);
        checkMagnitude(moveAssigned);
    }
}
