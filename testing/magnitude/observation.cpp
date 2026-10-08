#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitudeService/magnitude/observation.hpp"
#include "uLocalMagnitudeService/corrections/distance.hpp"
#include "uLocalMagnitudeService/corrections/hypocenter.hpp"
#include "uLocalMagnitudeService/corrections/stationLocation.hpp"
#include "uLocalMagnitudeService/magnitude/amplitude.hpp"
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/amplitude.pb.h"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/hypocenter.pb.h"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/station_amplitude_measurement.pb.h"
#include "uLocalMagnitudeServiceAPI/v1/magnitude/station_location.pb.h"

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

TEST_CASE("ULocalMagnitudeService::Magnitude::Observation - protobuf",
          "[observation]")
{
    namespace API = ULocalMagnitudeServiceAPI::V1::Magnitude;
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    const auto amplitudeMessage = [](const std::string &network,
                                     const std::string &station,
                                     const std::string &channel,
                                     const std::string &locationCode,
                                     const double value,
                                     const API::Amplitude_Units units)
    {
        API::Amplitude result;
        auto *identifier = result.mutable_stream_identifier();
        identifier->set_network(network);
        identifier->set_station(station);
        identifier->set_channel(channel);
        identifier->set_location_code(locationCode);
        result.set_value(value);
        result.set_units(units);
        return result;
    };
    // Roughly evid 80157466 and UU.CCUT
    API::Hypocenter hypocenter;
    hypocenter.set_latitude(39.6);
    hypocenter.set_longitude(-111.4);
    hypocenter.set_depth(7000);
    API::StationAmplitudeMeasurement message;
    *message.mutable_amplitude_stream_1()
        = amplitudeMessage("UU", "CCUT", "HHE", "01", 0.3459206596016884,
                           API::Amplitude_Units_MILLIMETERS);
    *message.mutable_amplitude_stream_2()
        = amplitudeMessage("UU", "CCUT", "HHN", "01", 0.15187045093625784,
                           API::Amplitude_Units_MILLIMETERS);
    message.mutable_station_location()->set_latitude(40.16);
    message.mutable_station_location()->set_longitude(-111.87);
    message.mutable_station_location()->set_elevation(1500);
    // The distance the observation should end up with
    const auto expectedDistance = [](const API::Hypocenter &source,
                                     const API::StationLocation &receiver)
    {
        return ULocalMagnitudeService::Corrections::Distance
               ::computeEpicentralDistance(
                   ULocalMagnitudeService::Corrections::Hypocenter {source},
                   ULocalMagnitudeService::Corrections::StationLocation
                       {receiver});
    };

    SECTION("From messages")
    {
        const Observation observation{message, hypocenter};
        checkAmplitudes(observation);
        REQUIRE(observation.getDepth() == 7000);
        const auto distance = observation.getEpicentralDistance();
        REQUIRE(distance
             == expectedDistance(hypocenter, message.station_location()));
        // About 74 km
        REQUIRE(distance > 70000);
        REQUIRE(distance < 80000);
    }

    SECTION("The distance is the geodesic between the hypocenter and station")
    {
        // One degree along the equator is exactly a*pi/180
        API::Hypocenter onEquator;
        onEquator.set_latitude(0);
        onEquator.set_longitude(0);
        onEquator.set_depth(10000);
        auto oneDegreeEast = message;
        oneDegreeEast.mutable_station_location()->set_latitude(0);
        oneDegreeEast.mutable_station_location()->set_longitude(1);
        REQUIRE_THAT((Observation {oneDegreeEast, onEquator})
                         .getEpicentralDistance(),
                     Catch::Matchers::WithinAbs(111319.49079327358, 1.e-6));
        // Depth and elevation don't change an epicentral distance
        auto deeper = onEquator;
        deeper.set_depth(30000);
        oneDegreeEast.mutable_station_location()->set_elevation(3000);
        REQUIRE_THAT((Observation {oneDegreeEast, deeper})
                         .getEpicentralDistance(),
                     Catch::Matchers::WithinAbs(111319.49079327358, 1.e-6));
        // A station on top of the source
        auto atSource = message;
        atSource.mutable_station_location()->set_latitude(39.6);
        atSource.mutable_station_location()->set_longitude(248.6);
        REQUIRE((Observation {atSource, hypocenter}).getEpicentralDistance()
             == 0);
    }

    SECTION("Amplitudes in centimeters are converted")
    {
        // This is how AQMS stores them
        *message.mutable_amplitude_stream_1()
            = amplitudeMessage("UU", "CCUT", "HHE", "01",
                               0.03459206596016884,
                               API::Amplitude_Units_CENTIMETERS);
        *message.mutable_amplitude_stream_2()
            = amplitudeMessage("UU", "CCUT", "HHN", "01",
                               0.015187045093625784,
                               API::Amplitude_Units_CENTIMETERS);
        checkAmplitudes(Observation {message, hypocenter});
    }

    SECTION("Amplitudes are kept in the order given")
    {
        auto swapped = message;
        *swapped.mutable_amplitude_stream_1() = message.amplitude_stream_2();
        *swapped.mutable_amplitude_stream_2() = message.amplitude_stream_1();
        const Observation observation{swapped, hypocenter};
        REQUIRE(observation.getAmplitudes().first.getName()
             == "UU.CCUT.HHN.01");
        REQUIRE(observation.getAmplitudes().second.getName()
             == "UU.CCUT.HHE.01");
    }

    SECTION("Both amplitudes are required")
    {
        auto noFirst = message;
        noFirst.clear_amplitude_stream_1();
        REQUIRE_THROWS_AS((Observation {noFirst, hypocenter}),
                          std::invalid_argument);

        auto noSecond = message;
        noSecond.clear_amplitude_stream_2();
        REQUIRE_THROWS_AS((Observation {noSecond, hypocenter}),
                          std::invalid_argument);
    }

    SECTION("Invalid amplitudes are rejected")
    {
        auto unknownUnits = message;
        unknownUnits.mutable_amplitude_stream_1()->set_units(
            API::Amplitude_Units_UNKNOWN);
        REQUIRE_THROWS_AS((Observation {unknownUnits, hypocenter}),
                          std::invalid_argument);

        auto zeroValue = message;
        zeroValue.mutable_amplitude_stream_2()->set_value(0);
        REQUIRE_THROWS_AS((Observation {zeroValue, hypocenter}),
                          std::invalid_argument);

        auto noStation = message;
        noStation.mutable_amplitude_stream_1()
                 ->mutable_stream_identifier()->clear_station();
        REQUIRE_THROWS_AS((Observation {noStation, hypocenter}),
                          std::invalid_argument);
    }

    SECTION("Mismatched amplitudes are rejected")
    {
        const auto mismatched = [&](const API::Amplitude &second)
        {
            auto result = message;
            *result.mutable_amplitude_stream_2() = second;
            return result;
        };
        const auto units = API::Amplitude_Units_MILLIMETERS;
        // Same channel twice
        REQUIRE_THROWS_AS(
            (Observation {mismatched(message.amplitude_stream_1()),
                          hypocenter}),
            std::invalid_argument);
        // Different stations
        REQUIRE_THROWS_AS(
            (Observation {mismatched(amplitudeMessage("UU", "LCMT", "HHN",
                                                      "01", 0.15, units)),
                          hypocenter}),
            std::invalid_argument);
        // Same station name on another network
        REQUIRE_THROWS_AS(
            (Observation {mismatched(amplitudeMessage("WY", "CCUT", "HHN",
                                                      "01", 0.15, units)),
                          hypocenter}),
            std::invalid_argument);
        // Different location codes
        REQUIRE_THROWS_AS(
            (Observation {mismatched(amplitudeMessage("UU", "CCUT", "HHN",
                                                      "02", 0.15, units)),
                          hypocenter}),
            std::invalid_argument);
        // Different sensors - e.g., MPU's ENN with HHE in evid 80157946
        REQUIRE_THROWS_AS(
            (Observation {mismatched(amplitudeMessage("UU", "CCUT", "ENN",
                                                      "01", 0.15, units)),
                          hypocenter}),
            std::invalid_argument);
    }

    SECTION("A complete, valid station location is required")
    {
        auto noLocation = message;
        noLocation.clear_station_location();
        REQUIRE_THROWS_AS((Observation {noLocation, hypocenter}),
                          std::invalid_argument);

        auto noLatitude = message;
        noLatitude.mutable_station_location()->clear_latitude();
        REQUIRE_THROWS_AS((Observation {noLatitude, hypocenter}),
                          std::invalid_argument);

        auto noLongitude = message;
        noLongitude.mutable_station_location()->clear_longitude();
        REQUIRE_THROWS_AS((Observation {noLongitude, hypocenter}),
                          std::invalid_argument);

        auto noElevation = message;
        noElevation.mutable_station_location()->clear_elevation();
        REQUIRE_THROWS_AS((Observation {noElevation, hypocenter}),
                          std::invalid_argument);

        auto badLatitude = message;
        badLatitude.mutable_station_location()->set_latitude(90.5);
        REQUIRE_THROWS_AS((Observation {badLatitude, hypocenter}),
                          std::invalid_argument);

        auto badLongitude = message;
        badLongitude.mutable_station_location()->set_longitude(
            std::numeric_limits<double>::quiet_NaN());
        REQUIRE_THROWS_AS((Observation {badLongitude, hypocenter}),
                          std::invalid_argument);

        auto badElevation = message;
        badElevation.mutable_station_location()->set_elevation(9000);
        REQUIRE_THROWS_AS((Observation {badElevation, hypocenter}),
                          std::invalid_argument);
    }

    SECTION("A complete, valid hypocenter is required")
    {
        auto noLatitude = hypocenter;
        noLatitude.clear_latitude();
        REQUIRE_THROWS_AS((Observation {message, noLatitude}),
                          std::invalid_argument);

        auto noLongitude = hypocenter;
        noLongitude.clear_longitude();
        REQUIRE_THROWS_AS((Observation {message, noLongitude}),
                          std::invalid_argument);

        auto noDepth = hypocenter;
        noDepth.clear_depth();
        REQUIRE_THROWS_AS((Observation {message, noDepth}),
                          std::invalid_argument);

        REQUIRE_THROWS_AS((Observation {message, API::Hypocenter {}}),
                          std::invalid_argument);

        auto badLatitude = hypocenter;
        badLatitude.set_latitude(-90.5);
        REQUIRE_THROWS_AS((Observation {message, badLatitude}),
                          std::invalid_argument);

        auto badLongitude = hypocenter;
        badLongitude.set_longitude(std::numeric_limits<double>::infinity());
        REQUIRE_THROWS_AS((Observation {message, badLongitude}),
                          std::invalid_argument);
    }

    SECTION("Depth must be valid")
    {
        for (const double depth : {-8600.0, 900000.0})
        {
            auto valid = hypocenter;
            valid.set_depth(depth);
            REQUIRE((Observation {message, valid}).getDepth() == depth);
        }
        for (const double depth :
                 {-8601.0, 900001.0,
                  std::numeric_limits<double>::quiet_NaN(),
                  std::numeric_limits<double>::infinity()})
        {
            INFO("Depth: " << depth);
            auto bad = hypocenter;
            bad.set_depth(depth);
            REQUIRE_THROWS_AS((Observation {message, bad}),
                              std::invalid_argument);
        }
    }

    SECTION("Round trip through the wire format")
    {
        API::StationAmplitudeMeasurement parsedMeasurement;
        REQUIRE(parsedMeasurement.ParseFromString(message.SerializeAsString()));
        API::Hypocenter parsedHypocenter;
        REQUIRE(parsedHypocenter.ParseFromString(
            hypocenter.SerializeAsString()));
        const Observation observation{parsedMeasurement, parsedHypocenter};
        checkAmplitudes(observation);
        REQUIRE(observation.getDepth() == 7000);
        REQUIRE(observation.getEpicentralDistance()
             == expectedDistance(hypocenter, message.station_location()));
    }
}
