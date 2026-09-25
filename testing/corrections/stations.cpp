#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitudeService/corrections/station.hpp"
#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"
#include "uLocalMagnitudeService/corrections/stationOptions.hpp"
#include "uLocalMagnitudeService/corrections/stations.hpp"
#include "uLocalMagnitudeService/corrections/stationsSet.hpp"

using namespace ULocalMagnitudeService::Corrections;

namespace
{

// (network, station, correction)
const std::vector<std::tuple<std::string, std::string, double>> corrections
{
    {"WY", "YEE", -0.09},
    {"WY", "YFT",  0.18},
    {"WY", "YHB",  0.06},
    {"WY", "YHR", -0.19},
    {"WY", "YMR", -0.10}
};

// Stations with no correction provided
const std::vector<std::pair<std::string, std::string>> misses
{
    {"WY", "YGC"},
    {"WY", "YBB"}
};

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
StationIdentifier makeIdentifier(const std::string &network,
                                 const std::string &station)
{
    StationIdentifier identifier;
    identifier.setNetwork(network);
    identifier.setStation(station);
    return identifier;
}

StationsSet makeStationsSet()
{
    StationsSet set;
    for (const auto &[network, station, correction] : corrections)
    {
        StationOptions options;
        options.setIdentifier(makeIdentifier(network, station));
        options.setCorrection(correction);
        if (!set.insert(Station {options}))
        {
            throw std::runtime_error(
                "Failed to insert "
              + makeIdentifier(network, station).toString());
        }
    }
    return set;
}

/// Checks every hit and miss through both lookup paths.
void checkStations(const Stations &stations)
{
    REQUIRE(stations.isInitialized());
    for (const auto &[network, station, correction] : corrections)
    {
        const auto byIdentifier = stations(makeIdentifier(network, station));
        REQUIRE(byIdentifier.has_value());
        CHECK_THAT(*byIdentifier,
                   Catch::Matchers::WithinAbs(correction, 1.e-14));

        const auto byName = stations(makeIdentifier(network, station).toString());
        REQUIRE(byName.has_value());
        CHECK_THAT(*byName, Catch::Matchers::WithinAbs(correction, 1.e-14));
    }
    for (const auto &[network, station] : misses)
    {
        const auto byIdentifier = stations(makeIdentifier(network, station));
        REQUIRE_FALSE(byIdentifier.has_value());
        CHECK(byIdentifier.error() == Stations::ErrorCode::StationDoesNotExist);

        const auto byName = stations(makeIdentifier(network, station).toString());
        REQUIRE_FALSE(byName.has_value());
        CHECK(byName.error() == Stations::ErrorCode::StationDoesNotExist);
    }
    // The convenience map has exactly the hits and none of the misses
    const auto correctionsMap = stations.getCorrections();
    REQUIRE(correctionsMap.size() == corrections.size());
    for (const auto &[network, station, correction] : corrections)
    {
        const auto name = makeIdentifier(network, station).toString();
        REQUIRE(correctionsMap.contains(name));
        CHECK_THAT(correctionsMap.at(name),
                   Catch::Matchers::WithinAbs(correction, 1.e-14));
    }
    for (const auto &[network, station] : misses)
    {
        CHECK_FALSE(correctionsMap.contains(
            makeIdentifier(network, station).toString()));
    }
}

}

TEST_CASE("ULocalMagnitudeService::Corrections::Stations", "[stations]")
{
    const auto set = makeStationsSet();

    SECTION("Empty set is rejected")
    {
        REQUIRE_THROWS_AS(Stations {StationsSet {}}, std::invalid_argument);
    }

    SECTION("Hits and misses")
    {
        const Stations stations{set};
        checkStations(stations);
    }

    SECTION("Identifier lookup is cleaned up like the identifier")
    {
        // The identifier removes blanks and capitalizes so this is WY.YFT
        const Stations stations{set};
        const auto result = stations(makeIdentifier(" wy", "y f t"));
        REQUIRE(result.has_value());
        REQUIRE_THAT(*result, Catch::Matchers::WithinAbs(0.18, 1.e-14));
    }

    SECTION("Invalid station")
    {
        const Stations stations{set};

        // Empty name
        const auto emptyName = stations(std::string {});
        REQUIRE_FALSE(emptyName.has_value());
        REQUIRE(emptyName.error() == Stations::ErrorCode::InvalidStation);

        // Identifier missing its network, station, or both
        const auto emptyIdentifier = stations(StationIdentifier {});
        REQUIRE_FALSE(emptyIdentifier.has_value());
        REQUIRE(emptyIdentifier.error() == Stations::ErrorCode::InvalidStation);

        StationIdentifier networkOnly;
        networkOnly.setNetwork("WY");
        const auto noStation = stations(networkOnly);
        REQUIRE_FALSE(noStation.has_value());
        REQUIRE(noStation.error() == Stations::ErrorCode::InvalidStation);

        StationIdentifier stationOnly;
        stationOnly.setStation("YFT");
        const auto noNetwork = stations(stationOnly);
        REQUIRE_FALSE(noNetwork.has_value());
        REQUIRE(noNetwork.error() == Stations::ErrorCode::InvalidStation);
    }

    SECTION("Right station, wrong network")
    {
        const Stations stations{set};
        const auto result = stations(makeIdentifier("UU", "YFT"));
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == Stations::ErrorCode::StationDoesNotExist);
    }

    SECTION("Set is copied in")
    {
        // Adding to the set afterwards doesn't leak into the corrections
        auto mutableSet = set;
        const Stations stations{mutableSet};
        StationOptions options;
        options.setIdentifier(makeIdentifier("WY", "YGC"));
        options.setCorrection(0.3);
        REQUIRE(mutableSet.insert(Station {options}));
        checkStations(stations);
    }

    SECTION("Copy and move")
    {
        const Stations stations{set};

        // Copy constructor
        // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
        const Stations copy{stations};
        checkStations(copy);

        // Copy assignment
        StationsSet otherSet;
        StationOptions options;
        options.setIdentifier(makeIdentifier("UU", "CWU"));
        options.setCorrection(0.05);
        REQUIRE(otherSet.insert(Station {options}));
        Stations copyAssigned{otherSet};
        copyAssigned = copy;
        checkStations(copyAssigned);

        // Move constructor
        Stations moved{std::move(copyAssigned)};
        checkStations(moved);

        // Move assignment
        Stations moveAssigned{otherSet};
        moveAssigned = std::move(moved);
        checkStations(moveAssigned);
        // The old corrections are gone
        REQUIRE(moveAssigned(std::string {"UU.CWU"}).error() ==
                Stations::ErrorCode::StationDoesNotExist);
    }
}
