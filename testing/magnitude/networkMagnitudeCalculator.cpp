#include <array>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitudeService/magnitude/networkMagnitude.hpp"
#include "uLocalMagnitudeService/magnitude/networkMagnitudeCalculator.hpp"
#include "uLocalMagnitudeService/magnitude/networkMagnitudeCalculatorOptions.hpp"
#include "uLocalMagnitudeService/magnitude/amplitude.hpp"
#include "uLocalMagnitudeService/magnitude/observation.hpp"
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"
#include "uLocalMagnitudeService/corrections/distance.hpp"
#include "uLocalMagnitudeService/corrections/distanceOptions.hpp"
#include "uLocalMagnitudeService/corrections/station.hpp"
#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"
#include "uLocalMagnitudeService/corrections/stationOptions.hpp"
#include "uLocalMagnitudeService/corrections/stationsSet.hpp"
#include "../distanceTables.hpp"

using namespace ULocalMagnitudeService;
using Magnitude::NetworkMagnitudeCalculator;
using Magnitude::NetworkMagnitudeCalculatorOptions;

namespace
{
Corrections::Distance utahDistanceCorrections()
{
    const Testing::TemporaryIniFile iniFile("networkCalculatorUtah",
                                            Testing::utahIniSection());
    return Corrections::Distance {
        Corrections::DistanceOptions::fromInitializationFile(iniFile.path())};
}

Corrections::Distance yellowstoneDistanceCorrections()
{
    const Testing::TemporaryIniFile iniFile("networkCalculatorYellowstone",
                                            Testing::yellowstoneIniSection());
    return Corrections::Distance {
        Corrections::DistanceOptions::fromInitializationFile(iniFile.path())};
}

Corrections::StationIdentifier makeIdentifier(const std::string &network,
                                              const std::string &station)
{
    Corrections::StationIdentifier identifier;
    identifier.setNetwork(network);
    identifier.setStation(station);
    return identifier;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
Corrections::Station makeStation(const std::string &network,
                                 const std::string &station,
                                 const double correction)
{
    Corrections::StationOptions options;
    options.setIdentifier(makeIdentifier(network, station));
    options.setCorrection(correction);
    return Corrections::Station {options};
}

/// Station corrections for evid 80157466 - see
/// testing/data/amp-ml-uu80157466.csv.
Corrections::StationsSet utahStations()
{
    Corrections::StationsSet set;
    REQUIRE(set.insert(makeStation("UU", "CCUT", 0.31)));
    REQUIRE(set.insert(makeStation("UU", "LCMT", -0.12)));
    REQUIRE(set.insert(makeStation("UU", "PKCU", -0.32)));
    REQUIRE(set.insert(makeStation("UU", "SZCU", -0.1)));
    REQUIRE(set.insert(makeStation("UU", "VRUT", 0.1)));
    REQUIRE(set.insert(makeStation("UU", "ZNPU", -0.2)));
    return set;
}

Magnitude::Amplitude amplitude(const std::string &network,
                               const std::string &station,
                               const std::string &channel,
                               const double value)
{
    Magnitude::StreamIdentifier identifier;
    identifier.setNetwork(network);
    identifier.setStation(station);
    identifier.setChannel(channel);
    identifier.setLocationCode("01");
    Magnitude::Amplitude result;
    result.setIdentifier(identifier);
    result.setValue(value);
    return result;
}

/// Creates an observation at the given station.
Magnitude::Observation observation(const std::string &network,
                                   const std::string &station,
                                   const double epicentralDistance,
                                   const std::optional<double> &depth)
{
    Magnitude::Observation result;
    result.setAmplitudes(std::pair {amplitude(network, station, "HHE", 0.35),
                                    amplitude(network, station, "HHN", 0.15)});
    result.setEpicentralDistance(epicentralDistance);
    if (depth){result.setDepth(*depth);}
    return result;
}

NetworkMagnitudeCalculatorOptions utahOptions()
{
    NetworkMagnitudeCalculatorOptions options;
    options.setDistanceCorrections(utahDistanceCorrections());
    options.setStationCorrections(utahStations());
    return options;
}

void checkUtah(const NetworkMagnitudeCalculator &calculator)
{
    REQUIRE(calculator.isInitialized());
    const auto distance = calculator.getDistanceCorrections();
    REQUIRE(distance.getDistanceType() ==
            Corrections::DistanceOptions::Type::Epicentral);
    REQUIRE(distance(66775.14936328208) == 2.8);
    const auto ccut
        = calculator.getStationCorrection(makeIdentifier("UU", "CCUT"));
    REQUIRE(ccut.has_value());
    REQUIRE_THAT(*ccut, Catch::Matchers::WithinAbs(0.31, 1.e-14));
}
}

TEST_CASE("ULocalMagnitudeService::Magnitude::NetworkMagnitudeCalculator",
          "[networkMagnitudeCalculator]")
{
    SECTION("Requires valid options")
    {
        REQUIRE_THROWS_AS(NetworkMagnitudeCalculator(NetworkMagnitudeCalculatorOptions {}, nullptr),
                          std::invalid_argument);

        NetworkMagnitudeCalculatorOptions noDistance;
        noDistance.setStationCorrections(utahStations());
        REQUIRE_THROWS_AS(NetworkMagnitudeCalculator(noDistance, nullptr),
                          std::invalid_argument);

        NetworkMagnitudeCalculatorOptions noStations;
        noStations.setDistanceCorrections(utahDistanceCorrections());
        REQUIRE_THROWS_AS(NetworkMagnitudeCalculator(noStations, nullptr),
                          std::invalid_argument);
    }

    SECTION("Distance corrections")
    {
        const NetworkMagnitudeCalculator utah{utahOptions(), nullptr};
        checkUtah(utah);

        auto options = utahOptions();
        options.setDistanceCorrections(yellowstoneDistanceCorrections());
        const NetworkMagnitudeCalculator yellowstone{options, nullptr};
        REQUIRE(yellowstone.getDistanceCorrections().getDistanceType() ==
                Corrections::DistanceOptions::Type::Hypocentral);
    }

    SECTION("Station corrections")
    {
        const NetworkMagnitudeCalculator calculator{utahOptions(), nullptr};
        const auto expected = std::array
        {
            std::pair {"CCUT",  0.31}, std::pair {"LCMT", -0.12},
            std::pair {"PKCU", -0.32}, std::pair {"SZCU", -0.1},
            std::pair {"VRUT",  0.1},  std::pair {"ZNPU", -0.2}
        };
        for (const auto &[station, correction] : expected)
        {
            INFO("Station: " << station);
            const auto value
                = calculator.getStationCorrection(makeIdentifier("UU",
                                                                 station));
            REQUIRE(value.has_value());
            REQUIRE_THAT(*value, Catch::Matchers::WithinAbs(correction,
                                                            1.e-14));
        }
        // Unknown stations - including the same name on another network
        REQUIRE_FALSE(calculator.getStationCorrection(
                          makeIdentifier("UU", "CTU")).has_value());
        REQUIRE_FALSE(calculator.getStationCorrection(
                          makeIdentifier("WY", "CCUT")).has_value());
    }

    SECTION("Options are copied in")
    {
        auto options = utahOptions();
        const NetworkMagnitudeCalculator calculator{options, nullptr};
        options.setDistanceCorrections(yellowstoneDistanceCorrections());
        Corrections::StationsSet other;
        REQUIRE(other.insert(makeStation("WY", "YFT", 0.18)));
        options.setStationCorrections(other);
        checkUtah(calculator);
        REQUIRE_FALSE(calculator.getStationCorrection(
                          makeIdentifier("WY", "YFT")).has_value());
    }

    SECTION("Copy and move")
    {
        const NetworkMagnitudeCalculator calculator{utahOptions(), nullptr};

        // Copy constructor
        const NetworkMagnitudeCalculator copy{calculator};
        checkUtah(copy);

        // Copy assignment
        NetworkMagnitudeCalculator copyAssigned{utahOptions(), nullptr};
        copyAssigned = copy;
        checkUtah(copyAssigned);

        // Move constructor
        NetworkMagnitudeCalculator moved{std::move(copyAssigned)};
        checkUtah(moved);

        // Move assignment
        auto yellowstoneOptions = utahOptions();
        yellowstoneOptions.setDistanceCorrections(
            yellowstoneDistanceCorrections());
        NetworkMagnitudeCalculator moveAssigned{yellowstoneOptions, nullptr};
        moveAssigned = std::move(moved);
        checkUtah(moveAssigned);
    }

    SECTION("Given logger")
    {
        const auto logger = spdlog::stdout_color_mt("networkCalculatorTest");
        const NetworkMagnitudeCalculator calculator{utahOptions(), logger};
        checkUtah(calculator);
        spdlog::drop("networkCalculatorTest");
    }
}

TEST_CASE("ULocalMagnitudeService::Magnitude::NetworkMagnitudeCalculator - observations",
          "[networkMagnitudeCalculator]")
{
    constexpr double depth{7000};
    const NetworkMagnitudeCalculator utah{utahOptions(), nullptr};
    const auto ccut = observation("UU", "CCUT", 66775.14936328208, depth);
    const auto lcmt = observation("UU", "LCMT", 31787.222423682797, depth);

    SECTION("Valid observations")
    {
        REQUIRE_NOTHROW(utah(std::vector {ccut, lcmt}));
        // Utah is epicentral so the depth isn't needed
        REQUIRE_NOTHROW(utah(std::vector {
            observation("UU", "CCUT", 66775.14936328208, std::nullopt),
            observation("UU", "LCMT", 31787.222423682797, std::nullopt)}));
    }

    SECTION("No observations")
    {
        REQUIRE_THROWS_AS(utah(std::vector<Magnitude::Observation> {}),
                          std::invalid_argument);
    }

    SECTION("Incomplete observations")
    {
        Magnitude::Observation noAmplitudes;
        noAmplitudes.setEpicentralDistance(10000);
        noAmplitudes.setDepth(depth);
        REQUIRE_THROWS_AS(utah(std::vector {ccut, lcmt, noAmplitudes}),
                          std::invalid_argument);

        Magnitude::Observation noDistance;
        noDistance.setAmplitudes(
            std::pair {amplitude("UU", "PKCU", "HHE", 0.35),
                       amplitude("UU", "PKCU", "HHN", 0.15)});
        noDistance.setDepth(depth);
        REQUIRE_THROWS_AS(utah(std::vector {ccut, lcmt, noDistance}),
                          std::invalid_argument);
    }

    SECTION("The average")
    {
        // CCUT: log10(0.5*0.5*(3 + 1)) + 1.4 + 0.31 = 1.71 at 0 km
        // LCMT: log10(0.5*0.5*(30 + 10)) + 1.4 - 0.12 = 2.28 at 0 km
        Magnitude::Observation ccut0;
        ccut0.setAmplitudes(std::pair {amplitude("UU", "CCUT", "HHE", 3),
                                       amplitude("UU", "CCUT", "HHN", 1)});
        ccut0.setEpicentralDistance(0);
        Magnitude::Observation lcmt0;
        lcmt0.setAmplitudes(std::pair {amplitude("UU", "LCMT", "HHE", 30),
                                       amplitude("UU", "LCMT", "HHN", 10)});
        lcmt0.setEpicentralDistance(0);
        const auto networkMagnitude = utah(std::vector {ccut0, lcmt0});
        REQUIRE(networkMagnitude.hasValue());
        REQUIRE_THAT(networkMagnitude.getValue(),
                     Catch::Matchers::WithinAbs(0.5*(1.71 + 2.28), 1.e-12));
    }

    SECTION("Station magnitudes that can't be computed don't count")
    {
        // Past the 600 km Utah table
        const auto tooFar = observation("UU", "PKCU", 700000, depth);
        const auto withTooFar = utah(std::vector {ccut, lcmt, tooFar});
        REQUIRE_THAT(withTooFar.getValue(),
                     Catch::Matchers::WithinAbs(
                         utah(std::vector {ccut, lcmt}).getValue(), 1.e-14));
        // and can leave too few
        REQUIRE_THROWS_AS(utah(std::vector {ccut, tooFar}),
                          std::invalid_argument);
    }

    SECTION("Hypocentral corrections need the depth")
    {
        auto options = utahOptions();
        options.setDistanceCorrections(yellowstoneDistanceCorrections());
        const NetworkMagnitudeCalculator hypocentral{options, nullptr};
        REQUIRE_NOTHROW(hypocentral(std::vector {ccut, lcmt}));
        const auto noDepth
            = observation("UU", "PKCU", 20000, std::nullopt);
        REQUIRE_THROWS_AS(hypocentral(std::vector {ccut, lcmt, noDepth}),
                          std::invalid_argument);
    }

    SECTION("Need enough unique stations with corrections")
    {
        // The default minimum is 2
        REQUIRE_THROWS_AS(utah(std::vector {ccut}), std::invalid_argument);
        // A repeated station only counts once
        REQUIRE_THROWS_AS(utah(std::vector {ccut, ccut}),
                          std::invalid_argument);
        // Stations without a correction don't count
        const auto unknown = observation("UU", "CTU", 20000, depth);
        const auto otherNetwork = observation("WY", "CCUT", 20000, depth);
        REQUIRE_THROWS_AS(utah(std::vector {ccut, unknown, otherNetwork}),
                          std::invalid_argument);
        REQUIRE_NOTHROW(utah(std::vector {ccut, unknown, lcmt, ccut}));
        // Raise the bar
        auto options = utahOptions();
        options.setMinimumNumberOfStationMagnitudes(3);
        const NetworkMagnitudeCalculator strict{options, nullptr};
        REQUIRE_THROWS_AS(strict(std::vector {ccut, lcmt, ccut}),
                          std::invalid_argument);
        const auto pkcu = observation("UU", "PKCU", 65753.37585962987, depth);
        REQUIRE_NOTHROW(strict(std::vector {ccut, lcmt, pkcu}));
    }
}

TEST_CASE("ULocalMagnitudeService::Magnitude::NetworkMagnitudeCalculator - AQMS Utah",
          "[networkMagnitudeCalculatorAQMS]")
{
    // Evid 80157466 (magid 121968) - see testing/data/amp-ml-uu80157466.csv.
    // AQMS reported a network Ml of 2.04.  The amplitudes are in cm and
    // the distances are epicentral in meters.
    struct Record
    {
        const char *station;
        double amplitudeEastInCentimeters;
        double amplitudeNorthInCentimeters;
        double distanceInMeters;
    };
    constexpr std::array<Record, 6> records
    {{
        {"CCUT", 0.03459206596016884,  0.015187045093625784, 66775.14936328208},
        {"LCMT", 0.2695453315973282,   0.3341444283723831,   31787.222423682797},
        {"PKCU", 0.032379960641264915, 0.023217148147523403, 65753.37585962987},
        {"SZCU", 0.11622115224599838,  0.12265413627028465,  59897.98846614931},
        {"VRUT", 0.012941689230501652, 0.00795073457993567,  95134.13961579288},
        {"ZNPU", 0.12780731543898582,  0.16992953419685364,  36945.35548051267}
    }};
    constexpr double centimetersToMillimeters{10};

    // Scales the amplitudes at one station
    const auto makeObservations = [&](const std::string &scaledStation,
                                      const double scale)
    {
        std::vector<Magnitude::Observation> observations;
        for (const auto &record : records)
        {
            const double factor
                = record.station == scaledStation ? scale : 1;
            Magnitude::Observation observation;
            observation.setAmplitudes(
                std::pair {amplitude("UU", record.station, "HHE",
                                     factor*centimetersToMillimeters
                                    *record.amplitudeEastInCentimeters),
                           amplitude("UU", record.station, "HHN",
                                     factor*centimetersToMillimeters
                                    *record.amplitudeNorthInCentimeters)});
            observation.setEpicentralDistance(record.distanceInMeters);
            observations.push_back(std::move(observation));
        }
        return observations;
    };
    // Mean of the six station magnitudes - AQMS rounds this to 2.04
    constexpr double expectedMagnitude{2.042090974351979};
    const NetworkMagnitudeCalculator calculator{utahOptions(), nullptr};

    SECTION("Reproduces AQMS")
    {
        const auto networkMagnitude = calculator(makeObservations("", 1));
        REQUIRE(networkMagnitude.hasValue());
        REQUIRE_THAT(networkMagnitude.getValue(),
                     Catch::Matchers::WithinAbs(expectedMagnitude, 1.e-10));
    }

    SECTION("A wild station is averaged in, not thrown out")
    {
        // 1000x the amplitude is 3 magnitude units at one of six stations
        const auto networkMagnitude
            = calculator(makeObservations("PKCU", 1000));
        REQUIRE_THAT(networkMagnitude.getValue(),
                     Catch::Matchers::WithinAbs(expectedMagnitude + 3./6.,
                                                1.e-10));
    }
}
