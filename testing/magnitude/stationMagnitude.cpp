#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitudeService/magnitude/stationMagnitude.hpp"
#include "uLocalMagnitudeService/magnitude/amplitude.hpp"
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/amplitude.pb.h"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/station_magnitude.pb.h"

using namespace ULocalMagnitudeService::Magnitude;

namespace
{
constexpr double notANumber{std::numeric_limits<double>::quiet_NaN()};
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
        for (const auto bad : {notANumber, infinity, -infinity})
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

TEST_CASE("ULocalMagnitudeService::Magnitude::StationMagnitude - protobuf",
          "[stationMagnitude]")
{
    namespace API = ULocalMagnitudeServiceAPI::V1::Magnitude;

    SECTION("To message")
    {
        const auto message = makeMagnitude().toMessage<API::StationMagnitude> ();
        REQUIRE(message.has_amplitude_1());
        REQUIRE(message.has_amplitude_2());
        REQUIRE(message.amplitude_1().units()
             == API::Amplitude_Units_MILLIMETERS);
        REQUIRE(message.amplitude_1().value() == 0.3459206596016884);
        REQUIRE(message.amplitude_2().value() == 0.15187045093625784);
        REQUIRE(message.amplitude_1().stream_identifier().network() == "UU");
        REQUIRE(message.amplitude_1().stream_identifier().station() == "CCUT");
        REQUIRE(message.amplitude_1().stream_identifier().channel() == "HHE");
        REQUIRE(message.amplitude_1().stream_identifier().location_code()
             == "01");
        REQUIRE(message.amplitude_2().stream_identifier().channel() == "HHN");
        REQUIRE(message.value() == 2.204987145462916);
        REQUIRE(message.station_correction() == 0.31);
        REQUIRE(message.distance_correction() == 2.8);
    }

    SECTION("The uncorrected magnitude can be recovered from the message")
    {
        // As the proto documents:
        //   uncorrected = value - station_correction - distance_correction
        // and for a station magnitude uncorrected = log10(A_average/2).
        const auto message = makeMagnitude().toMessage<API::StationMagnitude> ();
        const double averageAmplitude
            = 0.5*(message.amplitude_1().value()
                 + message.amplitude_2().value());
        REQUIRE_THAT(message.value() - message.station_correction()
                   - message.distance_correction(),
                     Catch::Matchers::WithinAbs(
                         std::log10(0.5*averageAmplitude), 1.e-12));
    }

    SECTION("Amplitudes are written in the order given")
    {
        auto [east, north] = ccutAmplitudes();
        auto magnitude = makeMagnitude();
        magnitude.setAmplitudes(std::pair {north, east});
        const auto message = magnitude.toMessage<API::StationMagnitude> ();
        REQUIRE(message.amplitude_1().stream_identifier().channel() == "HHN");
        REQUIRE(message.amplitude_2().stream_identifier().channel() == "HHE");
    }

    SECTION("Zero and negative values are written, not dropped")
    {
        auto magnitude = makeMagnitude();
        magnitude.setValue(-0.5);
        magnitude.setStationCorrection(0);
        magnitude.setDistanceCorrection(0);
        const auto message = magnitude.toMessage<API::StationMagnitude> ();
        REQUIRE(message.has_value());
        REQUIRE(message.has_station_correction());
        REQUIRE(message.has_distance_correction());
        REQUIRE(message.value() == -0.5);
        REQUIRE(message.station_correction() == 0);
        REQUIRE(message.distance_correction() == 0);
    }

    SECTION("To message requires everything to be set")
    {
        REQUIRE_THROWS_AS(
            StationMagnitude {}.toMessage<API::StationMagnitude> (),
            std::runtime_error);

        StationMagnitude noAmplitudes;
        noAmplitudes.setValue(2.2);
        noAmplitudes.setStationCorrection(0.31);
        noAmplitudes.setDistanceCorrection(2.8);
        REQUIRE_THROWS_AS(noAmplitudes.toMessage<API::StationMagnitude> (),
                          std::runtime_error);

        StationMagnitude noValue;
        noValue.setAmplitudes(ccutAmplitudes());
        noValue.setStationCorrection(0.31);
        noValue.setDistanceCorrection(2.8);
        REQUIRE_THROWS_AS(noValue.toMessage<API::StationMagnitude> (),
                          std::runtime_error);

        StationMagnitude noStationCorrection;
        noStationCorrection.setAmplitudes(ccutAmplitudes());
        noStationCorrection.setValue(2.2);
        noStationCorrection.setDistanceCorrection(2.8);
        REQUIRE_THROWS_AS(
            noStationCorrection.toMessage<API::StationMagnitude> (),
            std::runtime_error);

        StationMagnitude noDistanceCorrection;
        noDistanceCorrection.setAmplitudes(ccutAmplitudes());
        noDistanceCorrection.setValue(2.2);
        noDistanceCorrection.setStationCorrection(0.31);
        REQUIRE_THROWS_AS(
            noDistanceCorrection.toMessage<API::StationMagnitude> (),
            std::runtime_error);
    }

    SECTION("Writing a message doesn't change the station magnitude")
    {
        const auto magnitude = makeMagnitude();
        const auto first = magnitude.toMessage<API::StationMagnitude> ();
        const auto second = magnitude.toMessage<API::StationMagnitude> ();
        REQUIRE(first.SerializeAsString() == second.SerializeAsString());
        checkMagnitude(magnitude);
    }

    SECTION("Round trip through the wire format")
    {
        API::StationMagnitude parsed;
        REQUIRE(parsed.ParseFromString(
            makeMagnitude().toMessage<API::StationMagnitude> ()
           .SerializeAsString()));
        REQUIRE(parsed.value() == 2.204987145462916);
        REQUIRE(parsed.station_correction() == 0.31);
        REQUIRE(parsed.distance_correction() == 2.8);
        // A client can rebuild the amplitudes
        const Amplitude first{parsed.amplitude_1()};
        const Amplitude second{parsed.amplitude_2()};
        REQUIRE(first.getName() == "UU.CCUT.HHE.01");
        REQUIRE(second.getName() == "UU.CCUT.HHN.01");
        REQUIRE(first.getValue() == 0.3459206596016884);
        REQUIRE(second.getValue() == 0.15187045093625784);
    }
}
