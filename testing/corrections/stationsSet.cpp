#include <string>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitude/corrections/station.hpp"
#include "uLocalMagnitude/corrections/stationIdentifier.hpp"
#include "uLocalMagnitude/corrections/stationOptions.hpp"
#include "uLocalMagnitude/corrections/stationsSet.hpp"

using namespace ULocalMagnitude::Corrections;

namespace
{
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
Station makeStation(const std::string &network,
                    const std::string &station,
                    const double correction)
{
    StationIdentifier identifier;
    identifier.setNetwork(network);
    identifier.setStation(station);
    StationOptions options;
    options.setIdentifier(identifier);
    options.setCorrection(correction);
    return Station {options};
}
}

TEST_CASE("ULocalMagnitude::Corrections::StationsSet", "[stationsSet]")
{
    const auto yft = makeStation("WY", "YFT", 0.18);
    const auto ymr = makeStation("WY", "YMR", -0.12);
    const auto cwu = makeStation("UU", "CWU", 0.05);

    SECTION("Defaults")
    {
        const StationsSet set;
        REQUIRE(set.empty());
        REQUIRE(set.getCorrections().empty());
    }

    SECTION("Insert")
    {
        StationsSet set;
        REQUIRE(set.insert(yft));
        REQUIRE_FALSE(set.empty());
        REQUIRE(set.insert(ymr));
        REQUIRE(set.insert(cwu));

        const auto corrections = set.getCorrections();
        REQUIRE(corrections.size() == 3);
        REQUIRE(corrections.contains("WY.YFT"));
        REQUIRE(corrections.contains("WY.YMR"));
        REQUIRE(corrections.contains("UU.CWU"));
        REQUIRE_THAT(corrections.at("WY.YFT")(),
                     Catch::Matchers::WithinAbs(0.18, 1.e-14));
        REQUIRE_THAT(corrections.at("WY.YMR")(),
                     Catch::Matchers::WithinAbs(-0.12, 1.e-14));
        REQUIRE_THAT(corrections.at("UU.CWU")(),
                     Catch::Matchers::WithinAbs(0.05, 1.e-14));
        // Keys match the station names
        for (const auto &[name, station] : corrections)
        {
            REQUIRE(name == station.getName());
        }
    }

    SECTION("Duplicate is not inserted by default")
    {
        StationsSet set;
        REQUIRE(set.insert(yft));
        REQUIRE_FALSE(set.insert(makeStation("WY", "YFT", 0.5)));
        const auto corrections = set.getCorrections();
        REQUIRE(corrections.size() == 1);
        REQUIRE_THAT(corrections.at("WY.YFT")(),
                     Catch::Matchers::WithinAbs(0.18, 1.e-14));
    }

    SECTION("Duplicate is detected after name cleanup")
    {
        // " w y" and "yft " are cleaned to WY.YFT so this is the same station
        StationsSet set;
        REQUIRE(set.insert(yft));
        REQUIRE_FALSE(set.insert(makeStation(" w y", "yft ", 0.5)));
        REQUIRE(set.getCorrections().size() == 1);
    }

    SECTION("Duplicate overwrites when requested")
    {
        StationsSet set;
        REQUIRE(set.insert(yft));
        REQUIRE(set.insert(ymr));
        REQUIRE(set.insert(makeStation("WY", "YFT", 0.5), true));
        const auto corrections = set.getCorrections();
        REQUIRE(corrections.size() == 2);
        REQUIRE_THAT(corrections.at("WY.YFT")(),
                     Catch::Matchers::WithinAbs(0.5, 1.e-14));
        // Other stations are untouched
        REQUIRE_THAT(corrections.at("WY.YMR")(),
                     Catch::Matchers::WithinAbs(-0.12, 1.e-14));
    }

    SECTION("Overwrite of a new station is a plain insert")
    {
        StationsSet set;
        REQUIRE(set.insert(yft, true));
        REQUIRE(set.getCorrections().size() == 1);
    }

    SECTION("Get a single correction")
    {
        StationsSet set;
        REQUIRE(set.insert(yft));
        REQUIRE(set.insert(cwu));
        const auto hit = set.getCorrection("WY.YFT");
        REQUIRE(hit.has_value());
        if (hit.has_value()) // Placates clang-tidy
        {
            REQUIRE(hit->getName() == "WY.YFT");
            REQUIRE_THAT((*hit)(), Catch::Matchers::WithinAbs(0.18, 1.e-14));
        }
        REQUIRE_FALSE(set.getCorrection("WY.YMR").has_value());
        REQUIRE_FALSE(set.getCorrection("").has_value());
        REQUIRE_FALSE(StationsSet {}.getCorrection("WY.YFT").has_value());
    }

    SECTION("Returned corrections are a copy")
    {
        StationsSet set;
        REQUIRE(set.insert(yft));
        auto corrections = set.getCorrections();
        corrections.clear();
        REQUIRE_FALSE(set.empty());
        REQUIRE(set.getCorrections().size() == 1);
    }

    SECTION("Copy and move")
    {
        StationsSet set;
        REQUIRE(set.insert(yft));
        REQUIRE(set.insert(cwu));

        const auto check = [](const StationsSet &result)
        {
            const auto corrections = result.getCorrections();
            REQUIRE(corrections.size() == 2);
            REQUIRE_THAT(corrections.at("WY.YFT")(),
                         Catch::Matchers::WithinAbs(0.18, 1.e-14));
            REQUIRE_THAT(corrections.at("UU.CWU")(),
                         Catch::Matchers::WithinAbs(0.05, 1.e-14));
        };

        // Copy constructor
        const StationsSet copy{set};
        check(copy);

        // Copy is deep: modifying the original doesn't touch the copy
        REQUIRE(set.insert(ymr));
        REQUIRE(set.insert(makeStation("WY", "YFT", 0.5), true));
        check(copy);

        // Copy assignment
        StationsSet copyAssigned;
        copyAssigned = copy;
        check(copyAssigned);

        // Move constructor
        StationsSet moved{std::move(copyAssigned)};
        check(moved);

        // Move assignment
        StationsSet moveAssigned;
        moveAssigned = std::move(moved);
        check(moveAssigned);
    }
}
