#include <array>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitudeService/magnitude/networkMagnitude.hpp"
#include "uLocalMagnitudeService/magnitude/networkMagnitudeCalculator.hpp"
#include "uLocalMagnitudeService/magnitude/residual.hpp"
#include "uLocalMagnitudeService/magnitude/stationMagnitude.hpp"
#include "uLocalMagnitudeService/magnitude/amplitude.hpp"
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"
#include "uLocalMagnitudeServiceAPI/magnitude/v1/network_magnitude_from_amplitudes_response.pb.h"

using namespace ULocalMagnitudeService::Magnitude;
using RejectReason = NetworkMagnitudeCalculator::StationCalculatorErrorCode;

namespace
{
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
Amplitude makeAmplitude(const std::string &station,
                        const std::string &channel,
                        const double value)
{
    StreamIdentifier identifier;
    identifier.setNetwork("UU");
    identifier.setStation(station);
    identifier.setChannel(channel);
    identifier.setLocationCode("01");
    Amplitude amplitude;
    amplitude.setIdentifier(identifier);
    amplitude.setValue(value);
    return amplitude;
}

Residual makeResidual(const std::string &station,
                      const double stationMagnitude,
                      const double networkMagnitude)
{
    StationMagnitude magnitude;
    magnitude.setAmplitudes(std::pair {makeAmplitude(station, "HHE", 0.3),
                                       makeAmplitude(station, "HHN", 0.1)});
    magnitude.setValue(stationMagnitude);
    magnitude.setStationCorrection(0.1);
    magnitude.setDistanceCorrection(2.8);
    Residual residual;
    residual.setValue(networkMagnitude - stationMagnitude);
    residual.setStationMagnitude(magnitude);
    return residual;
}

std::vector<std::pair<Residual, int>> makeResiduals()
{
    return std::vector {std::pair {makeResidual("CCUT", 2.2, 2.0), 1},
                        std::pair {makeResidual("LCMT", 1.8, 2.0), 3}};
}

void checkResiduals(const std::vector<std::pair<Residual, int>> &residuals)
{
    REQUIRE(residuals.size() == 2);
    REQUIRE(residuals.at(0).second == 1);
    REQUIRE(residuals.at(0).first.getStationMagnitude().getStationName()
         == "UU.CCUT");
    REQUIRE_THAT(residuals.at(0).first.getValue(),
                 Catch::Matchers::WithinAbs(-0.2, 1.e-14));
    REQUIRE(residuals.at(1).second == 3);
    REQUIRE(residuals.at(1).first.getStationMagnitude().getStationName()
         == "UU.LCMT");
    REQUIRE_THAT(residuals.at(1).first.getValue(),
                 Catch::Matchers::WithinAbs(0.2, 1.e-14));
}

std::vector<std::pair<RejectReason, int>> makeRejects()
{
    return std::vector {std::pair {RejectReason::ObservationHasNoCorrection, 2},
                        std::pair {RejectReason::StationTooFar, 4}};
}
}

TEST_CASE("ULocalMagnitudeService::Magnitude::NetworkMagnitude",
          "[networkMagnitude]")
{
    SECTION("Defaults")
    {
        const NetworkMagnitude magnitude;
        REQUIRE_FALSE(magnitude.hasValue());
        REQUIRE_THROWS_AS(magnitude.getValue(), std::runtime_error);
        REQUIRE_FALSE(magnitude.hasResiduals());
        REQUIRE_THROWS_AS(magnitude.getResiduals(), std::runtime_error);
        REQUIRE_THROWS_AS(magnitude.getResidualsReference(),
                          std::runtime_error);
        REQUIRE_FALSE(magnitude.hasRejectedObservations());
        REQUIRE_THROWS_AS(magnitude.getRejectedObservations(),
                          std::runtime_error);
    }

    SECTION("Value")
    {
        NetworkMagnitude magnitude;
        magnitude.setValue(2.04);
        REQUIRE(magnitude.hasValue());
        REQUIRE_THAT(magnitude.getValue(),
                     Catch::Matchers::WithinAbs(2.04, 1.e-14));
        // Negative magnitudes are fine
        magnitude.setValue(-1.2);
        REQUIRE_THAT(magnitude.getValue(),
                     Catch::Matchers::WithinAbs(-1.2, 1.e-14));
        // Bounds are inclusive
        REQUIRE_NOTHROW(magnitude.setValue(-10));
        REQUIRE_NOTHROW(magnitude.setValue(10));
    }

    SECTION("Invalid value")
    {
        NetworkMagnitude magnitude;
        REQUIRE_THROWS_AS(magnitude.setValue(-10.01), std::invalid_argument);
        REQUIRE_THROWS_AS(magnitude.setValue(10.01), std::invalid_argument);
        // A failed set doesn't leave a value behind
        REQUIRE_FALSE(magnitude.hasValue());
        // or clobber an existing one
        magnitude.setValue(3.1);
        REQUIRE_THROWS_AS(magnitude.setValue(11), std::invalid_argument);
        REQUIRE_THAT(magnitude.getValue(),
                     Catch::Matchers::WithinAbs(3.1, 1.e-14));
    }

    SECTION("Residuals")
    {
        NetworkMagnitude magnitude;
        magnitude.setResiduals(makeResiduals());
        REQUIRE(magnitude.hasResiduals());
        checkResiduals(magnitude.getResiduals());
        checkResiduals(magnitude.getResidualsReference());
        // Rvalue setter
        NetworkMagnitude moved;
        moved.setResiduals(std::move(makeResiduals()));
        checkResiduals(moved.getResiduals());
    }

    SECTION("Invalid residuals")
    {
        NetworkMagnitude magnitude;
        REQUIRE_THROWS_AS(
            magnitude.setResiduals(std::vector<std::pair<Residual, int>> {}),
            std::invalid_argument);

        Residual noValue;
        noValue.setStationMagnitude(
            makeResidual("CCUT", 2.2, 2).getStationMagnitude());
        REQUIRE_THROWS_AS(
            magnitude.setResiduals(std::vector {std::pair {noValue, 1}}),
            std::invalid_argument);

        Residual noStationMagnitude;
        noStationMagnitude.setValue(0.1);
        REQUIRE_THROWS_AS(
            magnitude.setResiduals(
                std::vector {std::pair {noStationMagnitude, 1}}),
            std::invalid_argument);

        // Observation numbers are 1-based
        const auto good = makeResidual("CCUT", 2.2, 2);
        REQUIRE_THROWS_AS(
            magnitude.setResiduals(std::vector {std::pair {good, 0}}),
            std::invalid_argument);
        REQUIRE_THROWS_AS(
            magnitude.setResiduals(std::vector {std::pair {good, -1}}),
            std::invalid_argument);
        // One bad residual spoils the batch
        REQUIRE_THROWS_AS(
            magnitude.setResiduals(std::vector {std::pair {good, 1},
                                                std::pair {good, 0}}),
            std::invalid_argument);
        REQUIRE_FALSE(magnitude.hasResiduals());

        // and doesn't clobber what was there
        magnitude.setResiduals(makeResiduals());
        REQUIRE_THROWS_AS(
            magnitude.setResiduals(std::vector {std::pair {good, 0}}),
            std::invalid_argument);
        checkResiduals(magnitude.getResiduals());
    }

    SECTION("Rejected observations")
    {
        NetworkMagnitude magnitude;
        magnitude.setRejectedObservations(makeRejects());
        REQUIRE(magnitude.hasRejectedObservations());
        REQUIRE(magnitude.getRejectedObservations() == makeRejects());
        // Nothing rejected is a legitimate answer
        magnitude.setRejectedObservations({});
        REQUIRE(magnitude.hasRejectedObservations());
        REQUIRE(magnitude.getRejectedObservations().empty());
    }

    SECTION("Invalid rejected observations")
    {
        NetworkMagnitude magnitude;
        REQUIRE_THROWS_AS(
            magnitude.setRejectedObservations(
                {std::pair {RejectReason::StationTooFar, 0}}),
            std::invalid_argument);
        REQUIRE_FALSE(magnitude.hasRejectedObservations());
        magnitude.setRejectedObservations(makeRejects());
        REQUIRE_THROWS_AS(
            magnitude.setRejectedObservations(
                {std::pair {RejectReason::StationTooFar, 5},
                 std::pair {RejectReason::StationTooFar, -1}}),
            std::invalid_argument);
        REQUIRE(magnitude.getRejectedObservations() == makeRejects());
    }

    SECTION("Copy and move")
    {
        NetworkMagnitude magnitude;
        magnitude.setValue(1.58);
        magnitude.setResiduals(makeResiduals());
        magnitude.setRejectedObservations(makeRejects());
        const auto check = [](const NetworkMagnitude &result)
        {
            REQUIRE(result.hasValue());
            REQUIRE_THAT(result.getValue(),
                         Catch::Matchers::WithinAbs(1.58, 1.e-14));
            checkResiduals(result.getResiduals());
            REQUIRE(result.getRejectedObservations() == makeRejects());
        };

        // Copy constructor
        const NetworkMagnitude copy{magnitude};
        check(copy);
        // Copy is deep
        magnitude.setValue(4);
        magnitude.setResiduals(
            std::vector {std::pair {makeResidual("PKCU", 1, 4), 9}});
        magnitude.setRejectedObservations({});
        check(copy);

        // Copy assignment
        NetworkMagnitude copyAssigned;
        copyAssigned = copy;
        check(copyAssigned);

        // Move constructor
        NetworkMagnitude moved{std::move(copyAssigned)};
        check(moved);

        // Move assignment
        NetworkMagnitude moveAssigned;
        moveAssigned = std::move(moved);
        check(moveAssigned);
    }
}

TEST_CASE("ULocalMagnitudeService::Magnitude::NetworkMagnitude - protobuf",
          "[networkMagnitude]")
{
    namespace API = ULocalMagnitudeServiceAPI::Magnitude::V1;
    using Response = API::NetworkMagnitudeFromAmplitudesResponse;
    using MessageType = Response::RejectedObservation;

    SECTION("Rejected observations to message")
    {
        NetworkMagnitude magnitude;
        magnitude.setRejectedObservations(makeRejects());
        const auto messages
            = magnitude.toRejectedObservationsMessage<MessageType> ();
        REQUIRE(messages.size() == 2);
        // Order and observation numbers are preserved
        REQUIRE(messages.at(0).input_observation_number() == 2);
        REQUIRE(messages.at(0).reason()
             == Response::REASON_NO_STATION_CORRECTION);
        REQUIRE(messages.at(1).input_observation_number() == 4);
        REQUIRE(messages.at(1).reason() == Response::REASON_TOO_FAR);
    }

    SECTION("Nothing rejected is an empty list")
    {
        NetworkMagnitude magnitude;
        magnitude.setRejectedObservations({});
        REQUIRE(magnitude.toRejectedObservationsMessage<MessageType> ()
               .empty());
    }

    SECTION("Rejected observations must be set")
    {
        REQUIRE_THROWS_AS(
            NetworkMagnitude {}.toRejectedObservationsMessage<MessageType> (),
            std::runtime_error);
    }

    SECTION("Reasons")
    {
        const auto toReason = [](const RejectReason reason)
        {
            NetworkMagnitude magnitude;
            magnitude.setRejectedObservations({std::pair {reason, 1}});
            const auto messages
                = magnitude.toRejectedObservationsMessage<MessageType> ();
            REQUIRE(messages.size() == 1);
            REQUIRE(messages.at(0).input_observation_number() == 1);
            return messages.at(0).reason();
        };
        REQUIRE(toReason(RejectReason::StationTooClose)
             == Response::REASON_TOO_CLOSE);
        REQUIRE(toReason(RejectReason::StationTooFar)
             == Response::REASON_TOO_FAR);
        REQUIRE(toReason(RejectReason::ObservationMissingAmplitudes)
             == Response::REASON_INVALID_AMPLITUDES);
        REQUIRE(toReason(RejectReason::ObservationHasNoCorrection)
             == Response::REASON_NO_STATION_CORRECTION);
        REQUIRE(toReason(RejectReason::Algorithm)
             == Response::REASON_ALGORITHM);
        REQUIRE(toReason(RejectReason::Uninitialized)
             == Response::REASON_ALGORITHM);
        // Distances and depths come from the server's station locations
        // and a validated hypocenter so these are on the server
        REQUIRE(toReason(RejectReason::NegativeDistance)
             == Response::REASON_ALGORITHM);
        REQUIRE(toReason(RejectReason::InvalidDepth)
             == Response::REASON_ALGORITHM);
        // The server sets the distance and depth from the station location
        // and hypocenter so missing either is on the server
        REQUIRE(toReason(RejectReason::ObservationMissingDistance)
             == Response::REASON_ALGORITHM);
        REQUIRE(toReason(RejectReason::ObservationMissingDepth)
             == Response::REASON_ALGORITHM);
    }

    SECTION("Every rejection has a reason")
    {
        // If a new error code is added it needs a reason too - update this
        // list when StationCalculatorErrorCode changes
        constexpr std::array allReasons
        {
            RejectReason::ObservationMissingAmplitudes,
            RejectReason::ObservationMissingDistance,
            RejectReason::ObservationMissingDepth,
            RejectReason::ObservationHasNoCorrection,
            RejectReason::NegativeDistance,
            RejectReason::StationTooClose,
            RejectReason::StationTooFar,
            RejectReason::InvalidDepth,
            RejectReason::Uninitialized,
            RejectReason::Algorithm
        };
        static_assert(static_cast<int> (RejectReason::Algorithm) + 1
                   == static_cast<int> (allReasons.size()),
                      "StationCalculatorErrorCode changed - update this test");
        std::vector<std::pair<RejectReason, int>> rejects;
        int observationNumber{0};
        for (const auto reason : allReasons)
        {
            observationNumber = observationNumber + 1;
            rejects.emplace_back(reason, observationNumber);
        }
        NetworkMagnitude magnitude;
        magnitude.setRejectedObservations(rejects);
        const auto messages
            = magnitude.toRejectedObservationsMessage<MessageType> ();
        REQUIRE(messages.size() == allReasons.size());
        for (const auto &message : messages)
        {
            INFO("Observation number: " << message.input_observation_number());
            REQUIRE(message.reason() != Response::REASON_UNSPECIFIED);
        }
    }
}
