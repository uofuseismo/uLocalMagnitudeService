#include <array>
#include <stdexcept>
#include <string>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitudeService/magnitude/networkMagnitudeCalculator.hpp"
#include "uLocalMagnitudeService/magnitude/networkOptions.hpp"
#include "uLocalMagnitudeService/corrections/distance.hpp"
#include "uLocalMagnitudeService/corrections/distanceOptions.hpp"
#include "uLocalMagnitudeService/corrections/station.hpp"
#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"
#include "uLocalMagnitudeService/corrections/stationOptions.hpp"
#include "uLocalMagnitudeService/corrections/stationsSet.hpp"
#include "../distanceTables.hpp"

using namespace ULocalMagnitudeService;
using Magnitude::NetworkMagnitudeCalculator;
using Magnitude::NetworkOptions;

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

NetworkOptions utahOptions()
{
    NetworkOptions options;
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
        REQUIRE_THROWS_AS(NetworkMagnitudeCalculator {NetworkOptions {}},
                          std::invalid_argument);

        NetworkOptions noDistance;
        noDistance.setStationCorrections(utahStations());
        REQUIRE_THROWS_AS(NetworkMagnitudeCalculator {noDistance},
                          std::invalid_argument);

        NetworkOptions noStations;
        noStations.setDistanceCorrections(utahDistanceCorrections());
        REQUIRE_THROWS_AS(NetworkMagnitudeCalculator {noStations},
                          std::invalid_argument);
    }

    SECTION("Distance corrections")
    {
        const NetworkMagnitudeCalculator utah{utahOptions()};
        checkUtah(utah);

        auto options = utahOptions();
        options.setDistanceCorrections(yellowstoneDistanceCorrections());
        const NetworkMagnitudeCalculator yellowstone{options};
        REQUIRE(yellowstone.getDistanceCorrections().getDistanceType() ==
                Corrections::DistanceOptions::Type::Hypocentral);
    }

    SECTION("Station corrections")
    {
        const NetworkMagnitudeCalculator calculator{utahOptions()};
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
        const NetworkMagnitudeCalculator calculator{options};
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
        const NetworkMagnitudeCalculator calculator{utahOptions()};

        // Copy constructor
        const NetworkMagnitudeCalculator copy{calculator};
        checkUtah(copy);

        // Copy assignment
        NetworkMagnitudeCalculator copyAssigned{utahOptions()};
        copyAssigned = copy;
        checkUtah(copyAssigned);

        // Move constructor
        NetworkMagnitudeCalculator moved{std::move(copyAssigned)};
        checkUtah(moved);

        // Move assignment
        auto yellowstoneOptions = utahOptions();
        yellowstoneOptions.setDistanceCorrections(
            yellowstoneDistanceCorrections());
        NetworkMagnitudeCalculator moveAssigned{yellowstoneOptions};
        moveAssigned = std::move(moved);
        checkUtah(moveAssigned);
    }
}
