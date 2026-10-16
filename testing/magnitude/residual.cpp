#include <stdexcept>
#include <string>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitudeService/magnitude/residual.hpp"
#include "uLocalMagnitudeService/magnitude/stationMagnitude.hpp"
#include "uLocalMagnitudeService/magnitude/amplitude.hpp"
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"
#include "uLocalMagnitudeServiceAPI/magnitude/v1/network_magnitude_from_amplitudes_response.pb.h"
#include "uLocalMagnitudeServiceAPI/magnitude/v1/station_magnitude.pb.h"

using namespace ULocalMagnitudeService::Magnitude;

namespace
{
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
Amplitude makeAmplitude(const std::string &network,
                        const std::string &station,
                        const std::string &channel,
                        const double value)
{
    StreamIdentifier identifier;
    identifier.setNetwork(network);
    identifier.setStation(station);
    identifier.setChannel(channel);
    identifier.setLocationCode("01");
    Amplitude amplitude;
    amplitude.setIdentifier(identifier);
    amplitude.setValue(value);
    return amplitude;
}

/// UU.CCUT for evid 80157466 in mm.
StationMagnitude makeStationMagnitude()
{
    StationMagnitude magnitude;
    magnitude.setAmplitudes(
        std::pair {makeAmplitude("UU", "CCUT", "HHE", 0.3459206596016884),
                   makeAmplitude("UU", "CCUT", "HHN", 0.15187045093625784)});
    magnitude.setValue(2.204987145462916);
    magnitude.setStationCorrection(0.31);
    magnitude.setDistanceCorrection(2.8);
    return magnitude;
}

Residual makeResidual()
{
    Residual residual;
    residual.setValue(2.04 - 2.204987145462916);
    residual.setStationMagnitude(makeStationMagnitude());
    return residual;
}

void checkResidual(const Residual &residual)
{
    REQUIRE(residual.hasValue());
    REQUIRE(residual.hasStationMagnitude());
    REQUIRE(residual.getValue() == 2.04 - 2.204987145462916);
    REQUIRE(residual.getStationMagnitude().getStationName() == "UU.CCUT");
    REQUIRE(residual.getStationMagnitudeReference().getValue()
         == 2.204987145462916);
}
}

TEST_CASE("ULocalMagnitudeService::Magnitude::Residual", "[residual]")
{
    SECTION("Defaults")
    {
        const Residual residual;
        REQUIRE_FALSE(residual.hasValue());
        REQUIRE_FALSE(residual.hasStationMagnitude());
        REQUIRE_THROWS_AS(residual.getValue(), std::runtime_error);
        REQUIRE_THROWS_AS(residual.getStationMagnitude(), std::runtime_error);
        REQUIRE_THROWS_AS(residual.getStationMagnitudeReference(),
                          std::runtime_error);
    }

    SECTION("Setters and getters")
    {
        checkResidual(makeResidual());
    }

    SECTION("Any residual value is fine")
    {
        Residual residual;
        residual.setValue(0);
        REQUIRE(residual.getValue() == 0);
        residual.setValue(-1.5);
        REQUIRE(residual.getValue() == -1.5);
        residual.setValue(1.5);
        REQUIRE(residual.getValue() == 1.5);
    }

    SECTION("The station magnitude must be complete")
    {
        Residual residual;
        StationMagnitude noValue;
        noValue.setAmplitudes(makeStationMagnitude().getAmplitudes());
        noValue.setStationCorrection(0.31);
        noValue.setDistanceCorrection(2.8);
        REQUIRE_THROWS_AS(residual.setStationMagnitude(noValue),
                          std::invalid_argument);

        StationMagnitude noAmplitudes;
        noAmplitudes.setValue(2.2);
        noAmplitudes.setStationCorrection(0.31);
        noAmplitudes.setDistanceCorrection(2.8);
        REQUIRE_THROWS_AS(residual.setStationMagnitude(noAmplitudes),
                          std::invalid_argument);

        StationMagnitude noDistanceCorrection;
        noDistanceCorrection.setAmplitudes(
            makeStationMagnitude().getAmplitudes());
        noDistanceCorrection.setValue(2.2);
        noDistanceCorrection.setStationCorrection(0.31);
        REQUIRE_THROWS_AS(residual.setStationMagnitude(noDistanceCorrection),
                          std::invalid_argument);

        StationMagnitude noStationCorrection;
        noStationCorrection.setAmplitudes(
            makeStationMagnitude().getAmplitudes());
        noStationCorrection.setValue(2.2);
        noStationCorrection.setDistanceCorrection(2.8);
        REQUIRE_THROWS_AS(residual.setStationMagnitude(noStationCorrection),
                          std::invalid_argument);

        REQUIRE_FALSE(residual.hasStationMagnitude());
        // A rejected station magnitude doesn't clobber an existing one
        residual.setStationMagnitude(makeStationMagnitude());
        REQUIRE_THROWS_AS(residual.setStationMagnitude(noValue),
                          std::invalid_argument);
        REQUIRE(residual.getStationMagnitude().getValue()
             == 2.204987145462916);
    }

    SECTION("Rvalue setter")
    {
        Residual residual;
        auto stationMagnitude = makeStationMagnitude();
        residual.setStationMagnitude(std::move(stationMagnitude));
        REQUIRE(residual.getStationMagnitude().getStationName() == "UU.CCUT");
    }

    SECTION("Copy and move")
    {
        auto residual = makeResidual();

        // Copy constructor
        const Residual copy{residual};
        checkResidual(copy);
        // Copy is deep
        residual.setValue(9);
        checkResidual(copy);

        // Copy assignment
        Residual copyAssigned;
        copyAssigned = copy;
        checkResidual(copyAssigned);

        // Move constructor
        Residual moved{std::move(copyAssigned)};
        checkResidual(moved);

        // Move assignment
        Residual moveAssigned;
        moveAssigned = std::move(moved);
        checkResidual(moveAssigned);
    }
}

TEST_CASE("ULocalMagnitudeService::Magnitude::Residual - protobuf",
          "[residual]")
{
    namespace API = ULocalMagnitudeServiceAPI::Magnitude::V1;
    using MessageType
        = API::NetworkMagnitudeFromAmplitudesResponse::StationMagnitudeAndResidual;

    SECTION("To message")
    {
        const auto residual = makeResidual();
        const auto message
            = residual.toStationMagnitudeAndResidualMessage<MessageType> ();
        REQUIRE(message.residual() == 2.04 - 2.204987145462916);
        REQUIRE(message.has_identifier());
        REQUIRE(message.identifier().network() == "UU");
        REQUIRE(message.identifier().station() == "CCUT");
        REQUIRE(message.has_station_magnitude());
        // The station magnitude is written exactly as the station magnitude
        // writes itself
        const auto expected
            = makeStationMagnitude().toMessage<API::StationMagnitude> ();
        REQUIRE(message.station_magnitude().SerializeAsString()
             == expected.SerializeAsString());
        REQUIRE(message.station_magnitude().value() == 2.204987145462916);
        REQUIRE(message.station_magnitude().station_correction() == 0.31);
        REQUIRE(message.station_magnitude().distance_correction() == 2.8);
        REQUIRE(message.station_magnitude().amplitude_1().value()
             == 0.3459206596016884);
        REQUIRE(message.station_magnitude().amplitude_2().value()
             == 0.15187045093625784);
    }

    SECTION("The station magnitude is recoverable from the residual")
    {
        const auto message
            = makeResidual().toStationMagnitudeAndResidualMessage<MessageType> ();
        // A client should be able to get the network magnitude back
        REQUIRE_THAT(message.station_magnitude().value() + message.residual(),
                     Catch::Matchers::WithinAbs(2.04, 1.e-14));
    }

    SECTION("Survives the wire")
    {
        const auto message
            = makeResidual().toStationMagnitudeAndResidualMessage<MessageType> ();
        MessageType received;
        REQUIRE(received.ParseFromString(message.SerializeAsString()));
        REQUIRE(received.residual() == message.residual());
        REQUIRE(received.identifier().network() == "UU");
        REQUIRE(received.identifier().station() == "CCUT");
        REQUIRE(received.station_magnitude().value() == 2.204987145462916);
    }

    SECTION("To message requires everything to be set")
    {
        REQUIRE_THROWS_AS(
            Residual {}.toStationMagnitudeAndResidualMessage<MessageType> (),
            std::runtime_error);
        Residual noStationMagnitude;
        noStationMagnitude.setValue(0.1);
        REQUIRE_THROWS_AS(
            noStationMagnitude.toStationMagnitudeAndResidualMessage
                <MessageType> (),
            std::runtime_error);
        Residual noValue;
        noValue.setStationMagnitude(makeStationMagnitude());
        REQUIRE_THROWS_AS(
            noValue.toStationMagnitudeAndResidualMessage<MessageType> (),
            std::runtime_error);
    }
}
