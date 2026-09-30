#include <stdexcept>
#include <string>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitudeService/magnitude/networkOptions.hpp"
#include "uLocalMagnitudeService/corrections/distance.hpp"
#include "uLocalMagnitudeService/corrections/distanceOptions.hpp"
#include "uLocalMagnitudeService/corrections/station.hpp"
#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"
#include "uLocalMagnitudeService/corrections/stationOptions.hpp"
#include "uLocalMagnitudeService/corrections/stationsSet.hpp"
#include "../distanceTables.hpp"

using namespace ULocalMagnitudeService;
using Magnitude::NetworkOptions;

namespace
{
Corrections::Distance utahDistanceCorrections()
{
    const Testing::TemporaryIniFile iniFile("networkOptionsUtah",
                                            Testing::utahIniSection());
    return Corrections::Distance {
        Corrections::DistanceOptions::fromInitializationFile(iniFile.path())};
}

Corrections::Distance yellowstoneDistanceCorrections()
{
    const Testing::TemporaryIniFile iniFile("networkOptionsYellowstone",
                                            Testing::yellowstoneIniSection());
    return Corrections::Distance {
        Corrections::DistanceOptions::fromInitializationFile(iniFile.path())};
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
Corrections::Station makeStation(const std::string &network,
                                 const std::string &station,
                                 const double correction)
{
    Corrections::StationIdentifier identifier;
    identifier.setNetwork(network);
    identifier.setStation(station);
    Corrections::StationOptions options;
    options.setIdentifier(identifier);
    options.setCorrection(correction);
    return Corrections::Station {options};
}

Corrections::StationsSet utahStations()
{
    Corrections::StationsSet set;
    REQUIRE(set.insert(makeStation("UU", "CCUT", 0.31)));
    REQUIRE(set.insert(makeStation("UU", "LCMT", -0.12)));
    return set;
}

Corrections::StationsSet yellowstoneStations()
{
    Corrections::StationsSet set;
    REQUIRE(set.insert(makeStation("WY", "YFT", 0.18)));
    return set;
}

/// Checks the options match those made by makeUtahOptions().
void checkUtahOptions(const NetworkOptions &options)
{
    REQUIRE(options.hasDistanceCorrections());
    REQUIRE(options.hasStationCorrections());
    const auto distance = options.getDistanceCorrections();
    REQUIRE(distance.getDistanceType() ==
            Corrections::DistanceOptions::Type::Epicentral);
    REQUIRE(distance(30000) == 2.1);
    const auto stations = options.getStationCorrections();
    REQUIRE(stations.getCorrections().size() == 2);
    const auto ccut = stations.getCorrection("UU.CCUT");
    REQUIRE(ccut.has_value());
    REQUIRE_THAT((*ccut)(), Catch::Matchers::WithinAbs(0.31, 1.e-14));
    REQUIRE(options.getMinimumNumberOfStationMagnitudes() == 3);
    REQUIRE(options.getStrategy() == NetworkOptions::Strategy::Average);
}

NetworkOptions makeUtahOptions()
{
    NetworkOptions options;
    options.setDistanceCorrections(utahDistanceCorrections());
    options.setStationCorrections(utahStations());
    options.setMinimumNumberOfStationMagnitudes(3);
    options.setStrategy(NetworkOptions::Strategy::Average);
    return options;
}
}

TEST_CASE("ULocalMagnitudeService::Magnitude::NetworkOptions",
          "[networkOptions]")
{
    SECTION("Defaults")
    {
        const NetworkOptions options;
        REQUIRE_FALSE(options.hasDistanceCorrections());
        REQUIRE_FALSE(options.hasStationCorrections());
        REQUIRE_THROWS_AS(options.getDistanceCorrections(),
                          std::runtime_error);
        REQUIRE_THROWS_AS(options.getStationCorrections(),
                          std::runtime_error);
        REQUIRE(options.getMinimumNumberOfStationMagnitudes() == 2);
        REQUIRE(options.getStrategy() == NetworkOptions::Strategy::Average);
    }

    SECTION("Distance corrections")
    {
        NetworkOptions options;
        options.setDistanceCorrections(utahDistanceCorrections());
        REQUIRE(options.hasDistanceCorrections());
        auto distance = options.getDistanceCorrections();
        REQUIRE(distance.getDistanceType() ==
                Corrections::DistanceOptions::Type::Epicentral);
        REQUIRE(distance(30000) == 2.1);
        // Setting again overwrites
        options.setDistanceCorrections(yellowstoneDistanceCorrections());
        distance = options.getDistanceCorrections();
        REQUIRE(distance.getDistanceType() ==
                Corrections::DistanceOptions::Type::Hypocentral);
        REQUIRE_THAT(distance(30000),
                     Catch::Matchers::WithinAbs(2.11, 1.e-12));
    }

    SECTION("Distance corrections are copied in")
    {
        NetworkOptions options;
        auto distance = utahDistanceCorrections();
        options.setDistanceCorrections(distance);
        distance = yellowstoneDistanceCorrections();
        REQUIRE(options.getDistanceCorrections().getDistanceType() ==
                Corrections::DistanceOptions::Type::Epicentral);
    }

    SECTION("Station corrections")
    {
        NetworkOptions options;
        options.setStationCorrections(utahStations());
        REQUIRE(options.hasStationCorrections());
        auto stations = options.getStationCorrections();
        REQUIRE(stations.getCorrections().size() == 2);
        REQUIRE(stations.getCorrection("UU.CCUT").has_value());
        REQUIRE(stations.getCorrection("UU.LCMT").has_value());
        // Setting again replaces the set rather than merging into it
        options.setStationCorrections(yellowstoneStations());
        stations = options.getStationCorrections();
        REQUIRE(stations.getCorrections().size() == 1);
        REQUIRE(stations.getCorrection("WY.YFT").has_value());
        REQUIRE_FALSE(stations.getCorrection("UU.CCUT").has_value());
    }

    SECTION("Station corrections are copied in")
    {
        NetworkOptions options;
        auto stations = utahStations();
        options.setStationCorrections(stations);
        REQUIRE(stations.insert(makeStation("UU", "PKCU", -0.32)));
        REQUIRE(options.getStationCorrections().getCorrections().size() == 2);
    }

    SECTION("Empty station corrections are rejected")
    {
        NetworkOptions options;
        REQUIRE_THROWS_AS(
            options.setStationCorrections(Corrections::StationsSet {}),
            std::invalid_argument);
        REQUIRE_FALSE(options.hasStationCorrections());
        // A rejected set preserves the previous one
        options.setStationCorrections(utahStations());
        REQUIRE_THROWS_AS(
            options.setStationCorrections(Corrections::StationsSet {}),
            std::invalid_argument);
        REQUIRE(options.getStationCorrections().getCorrections().size() == 2);
    }

    SECTION("Minimum number of station magnitudes")
    {
        NetworkOptions options;
        options.setMinimumNumberOfStationMagnitudes(1);
        REQUIRE(options.getMinimumNumberOfStationMagnitudes() == 1);
        options.setMinimumNumberOfStationMagnitudes(10);
        REQUIRE(options.getMinimumNumberOfStationMagnitudes() == 10);
        REQUIRE_THROWS_AS(options.setMinimumNumberOfStationMagnitudes(0),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(options.setMinimumNumberOfStationMagnitudes(-1),
                          std::invalid_argument);
        REQUIRE(options.getMinimumNumberOfStationMagnitudes() == 10);
    }

    SECTION("Strategy")
    {
        NetworkOptions options;
        options.setStrategy(NetworkOptions::Strategy::Average);
        REQUIRE(options.getStrategy() == NetworkOptions::Strategy::Average);
    }

    SECTION("Copy and move")
    {
        auto options = makeUtahOptions();

        // Copy constructor
        const NetworkOptions copy{options};
        checkUtahOptions(copy);

        // Copy is deep: modifying the original doesn't touch the copy
        options.setDistanceCorrections(yellowstoneDistanceCorrections());
        options.setStationCorrections(yellowstoneStations());
        options.setMinimumNumberOfStationMagnitudes(5);
        checkUtahOptions(copy);

        // Copy assignment
        NetworkOptions copyAssigned;
        copyAssigned = copy;
        checkUtahOptions(copyAssigned);

        // Move constructor
        NetworkOptions moved{std::move(copyAssigned)};
        checkUtahOptions(moved);

        // Move assignment
        NetworkOptions moveAssigned;
        moveAssigned = std::move(moved);
        checkUtahOptions(moveAssigned);
    }

    SECTION("Copying defaults doesn't need a distance correction")
    {
        // The implementation holds the distance corrections by pointer so
        // make sure a copy of an unset one is safe.
        const NetworkOptions defaults;
        const NetworkOptions copy{defaults};
        REQUIRE_FALSE(copy.hasDistanceCorrections());
        NetworkOptions copyAssigned;
        copyAssigned = defaults;
        REQUIRE_FALSE(copyAssigned.hasDistanceCorrections());
    }
}
