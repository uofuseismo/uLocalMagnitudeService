#include <cmath>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitudeService/corrections/distance.hpp"
#include "uLocalMagnitudeService/corrections/distanceOptions.hpp"
#include "uLocalMagnitudeService/corrections/hypocenter.hpp"
#include "uLocalMagnitudeService/corrections/stationLocation.hpp"
#include "../distanceTables.hpp"

using namespace ULocalMagnitudeService::Corrections;
using namespace ULocalMagnitudeService::Testing;

namespace
{
/// Loads the options for one of the UUSS tables.
DistanceOptions uussOptions(const std::string &table)
{
    const TemporaryIniFile iniFile(table,
                                   table == "Utah" ?
                                   utahIniSection() : yellowstoneIniSection());
    return DistanceOptions::fromInitializationFile(iniFile.path());
}

/// Creates a distance correction from one of the UUSS tables.
Distance fromUUSSTable(const std::string &table)
{
    return Distance {uussOptions(table)};
}

/// Requires the distance correction failed with the given error.
void requireError(const std::expected<double, Distance::ErrorCode> &result,
                  const Distance::ErrorCode expected)
{
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == expected);
}

void checkCorrections(const std::vector<std::pair<double, double>> &expected,
                      const std::vector<std::pair<double, double>> &actual)
{
    REQUIRE(expected.size() == actual.size());
    for (std::size_t i = 0; i < expected.size(); ++i)
    {
        CHECK_THAT(actual.at(i).first,
                   Catch::Matchers::WithinAbs(expected.at(i).first, 1.e-10));
        CHECK_THAT(actual.at(i).second,
                   Catch::Matchers::WithinAbs(expected.at(i).second, 1.e-10));
    }
}
}

TEST_CASE("ULocalMagnitudeService::Corrections::DistanceOptions", "[distanceOptions]")
{
    // Deliberately unsorted
    const std::vector<std::pair<double, double>> corrections
    {
        {100000, -0.5},
        {0,       1.2},
        {10000,   0.3},
        {50000,   0.0}
    };
    const std::vector<std::pair<double, double>> correctionsSorted
    {
        {0,       1.2},
        {10000,   0.3},
        {50000,   0.0},
        {100000, -0.5}
    };

    SECTION("Defaults")
    {
        const DistanceOptions options;
        REQUIRE_FALSE(options.hasCorrections());
        REQUIRE_FALSE(options.hasInterpolation());
        REQUIRE_FALSE(options.hasType());
        REQUIRE_THROWS_AS(options.getInterpolation(), std::runtime_error);
        REQUIRE_THROWS_AS(options.getType(), std::runtime_error);
        REQUIRE_THROWS_AS(options.getCorrections(), std::runtime_error);
        REQUIRE_THROWS_AS(options.getCorrectionsReference(),
                          std::runtime_error);
    }

    SECTION("Interpolation")
    {
        DistanceOptions options;
        options.setInterpolation(DistanceOptions::Interpolation::Linear);
        REQUIRE(options.hasInterpolation());
        REQUIRE(options.getInterpolation() ==
                DistanceOptions::Interpolation::Linear);
        options.setInterpolation(DistanceOptions::Interpolation::Nearest);
        REQUIRE(options.getInterpolation() ==
                DistanceOptions::Interpolation::Nearest);
    }

    SECTION("Distance type")
    {
        DistanceOptions options;
        options.setType(DistanceOptions::Type::Hypocentral);
        REQUIRE(options.hasType());
        REQUIRE(options.getType() == DistanceOptions::Type::Hypocentral);
        options.setType(DistanceOptions::Type::Epicentral);
        REQUIRE(options.getType() == DistanceOptions::Type::Epicentral);
    }

    SECTION("Corrections are sorted")
    {
        DistanceOptions options;
        REQUIRE_NOTHROW(options.setCorrections(corrections));
        REQUIRE(options.hasCorrections());
        checkCorrections(correctionsSorted, options.getCorrections());
        checkCorrections(correctionsSorted,
                         options.getCorrectionsReference());
    }

    SECTION("Single correction")
    {
        DistanceOptions options;
        REQUIRE_NOTHROW(options.setCorrections({{5000, 0.25}}));
        checkCorrections({{5000, 0.25}}, options.getCorrections());
    }

    SECTION("Resetting corrections replaces the table")
    {
        DistanceOptions options;
        options.setCorrections(corrections);
        options.setCorrections({{2000, 0.1}, {1000, 0.2}});
        checkCorrections({{1000, 0.2}, {2000, 0.1}},
                         options.getCorrections());
    }

    SECTION("Invalid corrections")
    {
        DistanceOptions options;
        // Empty
        REQUIRE_THROWS_AS(options.setCorrections({}), std::invalid_argument);
        // Negative distance
        REQUIRE_THROWS_AS(options.setCorrections({{-1, 0.1}, {10, 0.2}}),
                          std::invalid_argument);
        // Exact duplicate
        REQUIRE_THROWS_AS(options.setCorrections({{10, 0.1}, {10, 0.2}}),
                          std::invalid_argument);
        // Duplicate within the 10 cm tolerance (and unsorted)
        REQUIRE_THROWS_AS(
            options.setCorrections({{500, 0.1}, {10, 0.2}, {10.05, 0.3}}),
            std::invalid_argument);
        // Failed sets should not leave the options populated
        REQUIRE_FALSE(options.hasCorrections());
    }

    SECTION("Failed set preserves previous corrections")
    {
        DistanceOptions options;
        options.setCorrections(corrections);
        REQUIRE_THROWS_AS(options.setCorrections({{-1, 0.1}}),
                          std::invalid_argument);
        REQUIRE(options.hasCorrections());
        checkCorrections(correctionsSorted, options.getCorrections());
    }

    SECTION("Distances just outside tolerance are distinct")
    {
        DistanceOptions options;
        REQUIRE_NOTHROW(options.setCorrections({{10, 0.1}, {10.2, 0.2}}));
        REQUIRE(options.getCorrections().size() == 2);
    }

    SECTION("Copy and move")
    {
        DistanceOptions options;
        options.setInterpolation(DistanceOptions::Interpolation::Linear);
        options.setCorrections(corrections);

        // Copy constructor
        DistanceOptions copy(options);
        REQUIRE(copy.getInterpolation() ==
                DistanceOptions::Interpolation::Linear);
        checkCorrections(correctionsSorted, copy.getCorrections());

        // Copy is deep: modifying the original doesn't touch the copy
        options.setCorrections({{1, 2}});
        options.setInterpolation(DistanceOptions::Interpolation::Nearest);
        REQUIRE(copy.getInterpolation() ==
                DistanceOptions::Interpolation::Linear);
        checkCorrections(correctionsSorted, copy.getCorrections());

        // Copy assignment
        DistanceOptions copyAssigned;
        copyAssigned = copy;
        REQUIRE(copyAssigned.getInterpolation() ==
                DistanceOptions::Interpolation::Linear);
        checkCorrections(correctionsSorted, copyAssigned.getCorrections());

        // Move constructor
        DistanceOptions moved(std::move(copy));
        REQUIRE(moved.getInterpolation() ==
                DistanceOptions::Interpolation::Linear);
        checkCorrections(correctionsSorted, moved.getCorrections());

        // Move assignment
        DistanceOptions moveAssigned;
        moveAssigned = std::move(moved);
        REQUIRE(moveAssigned.getInterpolation() ==
                DistanceOptions::Interpolation::Linear);
        checkCorrections(correctionsSorted, moveAssigned.getCorrections());
    }
}

TEST_CASE("ULocalMagnitudeService::Corrections::DistanceOptions::fromInitializationFile",
          "[distanceOptions]")
{
    SECTION("Missing file")
    {
        REQUIRE_THROWS_AS(
            DistanceOptions::fromInitializationFile("/this/file/does/not/exist.ini"),
            std::invalid_argument);
    }

    SECTION("UUSS tables")
    {
        // Utah is nearest neighbor on epicentral distance
        const auto utah = uussOptions("Utah");
        REQUIRE(utah.getInterpolation() ==
                DistanceOptions::Interpolation::Nearest);
        REQUIRE(utah.getType() == DistanceOptions::Type::Epicentral);
        const auto &utahTable = utah.getCorrectionsReference();
        REQUIRE(utahTable.size() == 70);
        checkCorrections(utahCorrections, utahTable);
        REQUIRE(utahTable.front() == std::pair {0.0, 1.4});
        REQUIRE(utahTable.back() == std::pair {600000.0, 4.9});

        // Yellowstone is linear on hypocentral distance
        const auto yellowstone = uussOptions("Yellowstone");
        REQUIRE(yellowstone.getInterpolation() ==
                DistanceOptions::Interpolation::Linear);
        REQUIRE(yellowstone.getType() == DistanceOptions::Type::Hypocentral);
        const auto &yellowstoneTable = yellowstone.getCorrectionsReference();
        REQUIRE(yellowstoneTable.size() == 39);
        checkCorrections(yellowstoneCorrections, yellowstoneTable);
        REQUIRE(yellowstoneTable.front() == std::pair {3000.0, 0.64});
        REQUIRE(yellowstoneTable.back() == std::pair {180000.0, 3.67});
    }

    SECTION("Interpolation and distance type are case insensitive")
    {
        const TemporaryIniFile iniFile("caseInsensitive",
                                       "[DistanceCorrections]\n"
                                       "interpolation = LiNeAr\n"
                                       "distanceType = HyPoCeNtRaL\n"
                                       "distance_correction_1 = 0, 1.0\n");
        const auto options = DistanceOptions::fromInitializationFile(iniFile.path());
        REQUIRE(options.getInterpolation() ==
                DistanceOptions::Interpolation::Linear);
        REQUIRE(options.getType() == DistanceOptions::Type::Hypocentral);
    }

    SECTION("Custom table in a custom section")
    {
        // Mixes the separators a user might reasonably type and is out of
        // order.
        const TemporaryIniFile iniFile("customTable",
                                       "[MyCorrections]\n"
                                       "interpolation = linear\n"
                                       "distanceType = epicentral\n"
                                       "distance_correction_1 = 10000, 2.0\n"
                                       "distance_correction_2 =   0    1.0\n"
                                       "distance_correction_3 = 30000\t4.0\n"
                                       "distance_correction_4 = 20000 , 3.0\n");
        const auto options
            = DistanceOptions::fromInitializationFile(iniFile.path(), "MyCorrections");
        REQUIRE(options.getInterpolation() ==
                DistanceOptions::Interpolation::Linear);
        REQUIRE(options.getType() == DistanceOptions::Type::Epicentral);
        checkCorrections({{0, 1.0}, {10000, 2.0}, {20000, 3.0}, {30000, 4.0}},
                         options.getCorrections());
    }

    SECTION("Invalid interpolation")
    {
        const TemporaryIniFile iniFile("badInterpolation",
                                       "[DistanceCorrections]\n"
                                       "interpolation = cubic\n"
                                       "distanceType = epicentral\n"
                                       "distance_correction_1 = 0, 1.0\n");
        REQUIRE_THROWS_AS(DistanceOptions::fromInitializationFile(iniFile.path()),
                          std::invalid_argument);
    }

    SECTION("Invalid distance type")
    {
        const TemporaryIniFile iniFile("badDistanceType",
                                       "[DistanceCorrections]\n"
                                       "interpolation = nearest\n"
                                       "distanceType = geodesic\n"
                                       "distance_correction_1 = 0, 1.0\n");
        REQUIRE_THROWS_AS(DistanceOptions::fromInitializationFile(iniFile.path()),
                          std::invalid_argument);
    }

    SECTION("Interpolation and distance type are required")
    {
        const TemporaryIniFile noInterpolation("noInterpolation",
                                               "[DistanceCorrections]\n"
                                               "distanceType = epicentral\n"
                                               "distance_correction_1 = 0, 1.0\n");
        REQUIRE_THROWS_AS(DistanceOptions::fromInitializationFile(noInterpolation.path()),
                          std::invalid_argument);
        const TemporaryIniFile noType("noDistanceType",
                                      "[DistanceCorrections]\n"
                                      "interpolation = nearest\n"
                                      "distance_correction_1 = 0, 1.0\n");
        REQUIRE_THROWS_AS(DistanceOptions::fromInitializationFile(noType.path()),
                          std::invalid_argument);
    }

    SECTION("The old hardwired table shortcuts are gone")
    {
        const TemporaryIniFile iniFile("oldShortcut",
                                       "[DistanceCorrections]\n"
                                       "interpolation = nearest\n"
                                       "distanceType = epicentral\n"
                                       "useUtahCorrections = true\n");
        REQUIRE_THROWS_AS(DistanceOptions::fromInitializationFile(iniFile.path()),
                          std::invalid_argument);
    }

    SECTION("Invalid table entry")
    {
        const TemporaryIniFile iniFile("badEntry",
                                       "[DistanceCorrections]\n"
                                       "interpolation = nearest\n"
                                       "distanceType = epicentral\n"
                                       "distance_correction_1 = 0, 1.0\n"
                                       "distance_correction_2 = 1 2 3\n");
        REQUIRE_THROWS_AS(DistanceOptions::fromInitializationFile(iniFile.path()),
                          std::invalid_argument);
    }

    SECTION("No table")
    {
        const TemporaryIniFile iniFile("noTable",
                                       "[DistanceCorrections]\n"
                                       "interpolation = nearest\n"
                                       "distanceType = epicentral\n");
        REQUIRE_THROWS_AS(DistanceOptions::fromInitializationFile(iniFile.path()),
                          std::invalid_argument);
    }
}

TEST_CASE("ULocalMagnitudeService::Corrections::Distance", "[distance]")
{
    // Distances in meters
    DistanceOptions options;
    options.setCorrections({{0, 1.0}, {10, 2.0}, {30, 4.0}});
    options.setInterpolation(DistanceOptions::Interpolation::Nearest);
    options.setType(DistanceOptions::Type::Epicentral);

    SECTION("Requires corrections, interpolation, and distance type")
    {
        REQUIRE_THROWS_AS(Distance {DistanceOptions {}}, std::runtime_error);

        DistanceOptions noInterpolation;
        noInterpolation.setCorrections({{0, 1.0}});
        noInterpolation.setType(DistanceOptions::Type::Epicentral);
        REQUIRE_THROWS_AS(Distance {noInterpolation}, std::runtime_error);

        DistanceOptions noType;
        noType.setCorrections({{0, 1.0}});
        noType.setInterpolation(DistanceOptions::Interpolation::Nearest);
        REQUIRE_THROWS_AS(Distance {noType}, std::runtime_error);
    }

    SECTION("Epicentral distance ignores the depth")
    {
        options.setInterpolation(DistanceOptions::Interpolation::Linear);
        const Distance distance{options};
        REQUIRE(distance.getDistanceType() ==
                DistanceOptions::Type::Epicentral);
        REQUIRE_THAT(distance(5, 0).value(),   Catch::Matchers::WithinAbs(1.5, 1.e-12));
        REQUIRE_THAT(distance(5, 20).value(),  Catch::Matchers::WithinAbs(1.5, 1.e-12));
        REQUIRE_THAT(distance(5, -20).value(), Catch::Matchers::WithinAbs(1.5, 1.e-12));
        requireError(distance(-1, 0), Distance::ErrorCode::NegativeDistance);
        // The depth isn't checked when it isn't used
        REQUIRE(distance(5, -8601).has_value());
        REQUIRE(distance(5, 900001).has_value());
    }

    SECTION("Hypocentral distance uses the depth")
    {
        options.setInterpolation(DistanceOptions::Interpolation::Linear);
        options.setType(DistanceOptions::Type::Hypocentral);
        const Distance distance{options};
        REQUIRE(distance.getDistanceType() ==
                DistanceOptions::Type::Hypocentral);
        // 3-4-5 triangle: hypocentral distance of 5 m
        REQUIRE_THAT(distance(3, 4).value(),  Catch::Matchers::WithinAbs(1.5, 1.e-12));
        REQUIRE_THAT(distance(4, 3).value(),  Catch::Matchers::WithinAbs(1.5, 1.e-12));
        // Above the datum is fine
        REQUIRE_THAT(distance(3, -4).value(), Catch::Matchers::WithinAbs(1.5, 1.e-12));
        REQUIRE_THAT(distance(5, 0).value(),  Catch::Matchers::WithinAbs(1.5, 1.e-12));
        // Directly above the source
        REQUIRE_THAT(distance(0, 20).value(), Catch::Matchers::WithinAbs(3.0, 1.e-12));
        requireError(distance(-1, 4), Distance::ErrorCode::NegativeDistance);
        // The depth can push an epicentrally close station past the table
        REQUIRE(distance(20, 0).has_value());
        requireError(distance(20, 25), Distance::ErrorCode::StationTooFar);
        // Depth bounds are checked before the distance
        requireError(distance(10, -8601),
                     Distance::ErrorCode::InvalidSourceDepth);
        requireError(distance(10, 900001),
                     Distance::ErrorCode::InvalidSourceDepth);
    }

    SECTION("Hypocentral depth bounds")
    {
        // Needs a table that reaches out past the deepest allowed source
        DistanceOptions wide;
        wide.setInterpolation(DistanceOptions::Interpolation::Linear);
        wide.setType(DistanceOptions::Type::Hypocentral);
        wide.setCorrections({{0, 1.0}, {1000000, 2.0}});
        const Distance distance{wide};
        REQUIRE(distance(10, -8600).has_value());
        REQUIRE(distance(10, 900000).has_value());
        requireError(distance(10, -8601),
                     Distance::ErrorCode::InvalidSourceDepth);
        requireError(distance(10, 900001),
                     Distance::ErrorCode::InvalidSourceDepth);
    }

    SECTION("Invalid distances")
    {
        const Distance distance{options};
        requireError(distance(-1.0), Distance::ErrorCode::NegativeDistance);
        // The table ends at 30 m
        REQUIRE(distance.getMaximumDistance() == 30);
        REQUIRE(distance(30.0) == 4.0);
        requireError(distance(30.001), Distance::ErrorCode::StationTooFar);
        requireError(distance(21000000.0), Distance::ErrorCode::StationTooFar);
    }

    SECTION("Maximum distance is the largest table distance")
    {
        DistanceOptions unsorted;
        unsorted.setCorrections({{30, 4.0}, {50, 5.0}, {0, 1.0}});
        unsorted.setInterpolation(DistanceOptions::Interpolation::Nearest);
        unsorted.setType(DistanceOptions::Type::Epicentral);
        const Distance distance{unsorted};
        REQUIRE(distance.getMaximumDistance() == 50);
        REQUIRE(distance(50) == 5.0);
        requireError(distance(51), Distance::ErrorCode::StationTooFar);
    }

    SECTION("Nearest")
    {
        options.setInterpolation(DistanceOptions::Interpolation::Nearest);
        const Distance distance{options};
        REQUIRE(distance(0) == 1.0);
        REQUIRE(distance(4) == 1.0);
        REQUIRE(distance(5) == 1.0);  // Tie goes to the closer-in node
        REQUIRE(distance(6) == 2.0);
        REQUIRE(distance(10) == 2.0);
        REQUIRE(distance(20) == 2.0); // Tie
        REQUIRE(distance(21) == 4.0);
        REQUIRE(distance(30) == 4.0);
        requireError(distance(1000), Distance::ErrorCode::StationTooFar);
    }

    SECTION("Linear")
    {
        options.setInterpolation(DistanceOptions::Interpolation::Linear);
        const Distance distance{options};
        REQUIRE_THAT(distance(0).value(),  Catch::Matchers::WithinAbs(1.0, 1.e-12));
        REQUIRE_THAT(distance(5).value(),  Catch::Matchers::WithinAbs(1.5, 1.e-12));
        REQUIRE_THAT(distance(10).value(), Catch::Matchers::WithinAbs(2.0, 1.e-12));
        REQUIRE_THAT(distance(20).value(), Catch::Matchers::WithinAbs(3.0, 1.e-12));
        REQUIRE_THAT(distance(25).value(), Catch::Matchers::WithinAbs(3.5, 1.e-12));
        REQUIRE_THAT(distance(30).value(), Catch::Matchers::WithinAbs(4.0, 1.e-12));
        requireError(distance(1000), Distance::ErrorCode::StationTooFar);
    }

    SECTION("Linear saturates below the first node")
    {
        DistanceOptions shifted;
        shifted.setInterpolation(DistanceOptions::Interpolation::Linear);
        shifted.setType(DistanceOptions::Type::Epicentral);
        shifted.setCorrections({{100, 1.0}, {200, 2.0}});
        const Distance distance{shifted};
        REQUIRE(distance(0) == 1.0);
        REQUIRE(distance(50) == 1.0);
    }

    SECTION("Single entry table is a constant")
    {
        for (const auto interpolation : {DistanceOptions::Interpolation::Nearest,
                                         DistanceOptions::Interpolation::Linear})
        {
            DistanceOptions single;
            single.setInterpolation(interpolation);
            single.setType(DistanceOptions::Type::Epicentral);
            single.setCorrections({{5000, 0.25}});
            const Distance distance{single};
            REQUIRE(distance(0) == 0.25);
            REQUIRE(distance(5000) == 0.25);
            requireError(distance(100000), Distance::ErrorCode::StationTooFar);
        }
    }

    SECTION("Copy and move")
    {
        options.setInterpolation(DistanceOptions::Interpolation::Linear);
        const Distance distance{options};
        // Exercising the copy constructor is the point
        // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
        const Distance copy{distance};
        REQUIRE_THAT(copy(5).value(), Catch::Matchers::WithinAbs(1.5, 1.e-12));
        Distance copyAssigned{options};
        copyAssigned = copy;
        REQUIRE_THAT(copyAssigned(5).value(), Catch::Matchers::WithinAbs(1.5, 1.e-12));
        Distance moved{std::move(copyAssigned)};
        REQUIRE_THAT(moved(5).value(), Catch::Matchers::WithinAbs(1.5, 1.e-12));
        Distance moveAssigned{options};
        moveAssigned = std::move(moved);
        REQUIRE_THAT(moveAssigned(5).value(), Catch::Matchers::WithinAbs(1.5, 1.e-12));
        checkCorrections({{0, 1.0}, {10, 2.0}, {30, 4.0}},
                         moveAssigned.getCorrections());
    }

    SECTION("Corrections table")
    {
        DistanceOptions unsorted;
        unsorted.setCorrections({{30, 4.0}, {0, 1.0}, {10, 2.0}});
        unsorted.setInterpolation(DistanceOptions::Interpolation::Nearest);
        unsorted.setType(DistanceOptions::Type::Epicentral);
        const Distance distance{unsorted};
        checkCorrections({{0, 1.0}, {10, 2.0}, {30, 4.0}},
                         distance.getCorrections());
        // Copy gets its own table
        // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
        const Distance copy{distance};
        checkCorrections({{0, 1.0}, {10, 2.0}, {30, 4.0}},
                         copy.getCorrections());
    }
}

TEST_CASE("ULocalMagnitudeService::Corrections::Distance - Utah and Yellowstone",
          "[distance]")
{
    // The lookup takes meters but the tables are written in kilometers.
    // Utah is nearest neighbor while Yellowstone is linear.
    const auto utahNearest = fromUUSSTable("Utah");
    const auto yellowstoneLinear = fromUUSSTable("Yellowstone");
    const auto atKm = [](const Distance &distance, const double kilometers)
                      {
                          return distance(kilometers*1.e3);
                      };

    SECTION("A tabulated distance takes its own correction")
    {
        REQUIRE(atKm(utahNearest, 0.0)   == 1.4);
        REQUIRE(atKm(utahNearest, 30.0)  == 2.1);
        REQUIRE(atKm(utahNearest, 220.0) == 3.65);
        REQUIRE(atKm(utahNearest, 600.0) == 4.9);
        REQUIRE(atKm(yellowstoneLinear, 3.0)   == 0.64);
        REQUIRE(atKm(yellowstoneLinear, 180.0) == 3.67);
    }

    SECTION("Between two entries the nearest one applies")
    {
        REQUIRE(atKm(utahNearest, 31.0)  == 2.1);  // 30 -> 35
        REQUIRE(atKm(utahNearest, 32.5)  == 2.1);  // Tie goes to 30
        REQUIRE(atKm(utahNearest, 32.6)  == 2.3);
        REQUIRE(atKm(utahNearest, 34.99) == 2.3);
        REQUIRE(atKm(utahNearest, 35.0)  == 2.3);
    }

    SECTION("The wide Utah gap at 70 km behaves like any other")
    {
        // There is no 75 km entry so 70 and 80 split the gap.
        REQUIRE(atKm(utahNearest, 70.0) == 2.8);
        REQUIRE(atKm(utahNearest, 75.0) == 2.8); // Tie
        REQUIRE(atKm(utahNearest, 75.1) == 2.9);
        REQUIRE(atKm(utahNearest, 80.0) == 2.9);
    }

    SECTION("Closer than the table starts takes the first correction")
    {
        // Utah starts at 0 so only Yellowstone can be undershot - it
        // starts at 3 km.
        REQUIRE(atKm(yellowstoneLinear, 0.0) == 0.64);
        REQUIRE(atKm(yellowstoneLinear, 2.9) == 0.64);
    }

    SECTION("Further than the table goes is too far")
    {
        REQUIRE(utahNearest.getMaximumDistance() == 600000);
        REQUIRE(yellowstoneLinear.getMaximumDistance() == 180000);
        requireError(atKm(utahNearest, 601.0),
                     Distance::ErrorCode::StationTooFar);
        requireError(atKm(utahNearest, 5000.0),
                     Distance::ErrorCode::StationTooFar);
        requireError(atKm(yellowstoneLinear, 181.0),
                     Distance::ErrorCode::StationTooFar);
        requireError(atKm(yellowstoneLinear, 5000.0),
                     Distance::ErrorCode::StationTooFar);
        // Yellowstone is hypocentral so the depth can push it past the table
        REQUIRE(yellowstoneLinear(179000, 0).has_value());
        requireError(yellowstoneLinear(179000, 20000),
                     Distance::ErrorCode::StationTooFar);
    }

    SECTION("The two regions are different curves, not an offset")
    {
        REQUIRE(atKm(utahNearest, 50.0) == 2.6);
        REQUIRE(atKm(yellowstoneLinear, 50.0) == 2.69);
        // Utah is the higher curve close in and the lower one far out -
        // they cross so no single offset relates them.
        REQUIRE(atKm(utahNearest, 15.0) == 1.6);
        REQUIRE(atKm(yellowstoneLinear, 15.0) == 1.33);
        REQUIRE(atKm(utahNearest, 150.0) == 3.3);
        REQUIRE(atKm(yellowstoneLinear, 150.0) == 3.5);
    }

    SECTION("The Yellowstone dip is preserved, not smoothed")
    {
        // The corrections rise to 3.17 at 80 km, fall to 3.06 at 110, then
        // rise again.  That is in the source table and a "fix" that made
        // it monotonic would be wrong.
        REQUIRE(atKm(yellowstoneLinear, 80.0)  == 3.17);
        REQUIRE(atKm(yellowstoneLinear, 110.0) == 3.06);
        REQUIRE(atKm(yellowstoneLinear, 140.0) == 3.37);
        REQUIRE(atKm(yellowstoneLinear, 110.0).value()
              < atKm(yellowstoneLinear, 80.0).value());
    }

    SECTION("Linear interpolation")
    {
        // Utah can still be linearly interpolated by changing the options
        auto utahLinearOptions = uussOptions("Utah");
        utahLinearOptions.setInterpolation(
            DistanceOptions::Interpolation::Linear);
        const Distance utahLinear{utahLinearOptions};
        // Nodes are unchanged
        REQUIRE_THAT(atKm(utahLinear, 30.0).value(),
                     Catch::Matchers::WithinAbs(2.1, 1.e-12));
        // Midpoints
        REQUIRE_THAT(atKm(utahLinear, 32.5).value(),
                     Catch::Matchers::WithinAbs(2.2, 1.e-12));
        REQUIRE_THAT(atKm(utahLinear, 72.5).value(),
                     Catch::Matchers::WithinAbs(2.825, 1.e-12));
        REQUIRE_THAT(atKm(yellowstoneLinear, 4.5).value(),
                     Catch::Matchers::WithinAbs(0.68, 1.e-12));
        // Saturates below the first node and stops at the last
        REQUIRE_THAT(atKm(yellowstoneLinear, 0.0).value(),
                     Catch::Matchers::WithinAbs(0.64, 1.e-12));
        REQUIRE_THAT(atKm(utahLinear, 600.0).value(),
                     Catch::Matchers::WithinAbs(4.9, 1.e-12));
        requireError(atKm(utahLinear, 700.0),
                     Distance::ErrorCode::StationTooFar);
    }

    SECTION("Negative distances are rejected")
    {
        requireError(utahNearest(-1.0), Distance::ErrorCode::NegativeDistance);
        requireError(utahNearest(-1.0, 0),
                     Distance::ErrorCode::NegativeDistance);
        REQUIRE(utahNearest(0.0).has_value());
    }
}

TEST_CASE("ULocalMagnitudeService::Corrections::Distance::computeEpicentralDistance",
          "[distance]")
{
    const auto hypocenter = [](const double latitude, const double longitude)
    {
        Hypocenter result;
        result.setLatitude(latitude);
        result.setLongitude(longitude);
        return result;
    };
    const auto station = [](const double latitude, const double longitude)
    {
        StationLocation result;
        result.setLatitude(latitude);
        result.setLongitude(longitude);
        return result;
    };

    SECTION("Same place")
    {
        REQUIRE(Distance::computeEpicentralDistance(hypocenter(39.6, -111.4),
                                                    station(39.6, -111.4))
             == 0);
    }

    SECTION("Along the equator")
    {
        // The equator is a geodesic so one degree is exactly a*pi/180
        REQUIRE_THAT(Distance::computeEpicentralDistance(hypocenter(0, 0),
                                                         station(0, 1)),
                     Catch::Matchers::WithinAbs(111319.49079327358, 1.e-6));
    }

    SECTION("Along a meridian")
    {
        // WGS84 quarter meridian
        REQUIRE_THAT(Distance::computeEpicentralDistance(hypocenter(0, 0),
                                                         station(90, 0)),
                     Catch::Matchers::WithinAbs(10001965.729, 1.e-3));
        // A short hop near Utah is the meridian radius of curvature times
        // the angle
        REQUIRE_THAT(Distance::computeEpicentralDistance(
                         hypocenter(40.00, -111.85), station(40.01, -111.85)),
                     Catch::Matchers::WithinAbs(1110.3473, 1.e-3));
    }

    SECTION("Symmetric")
    {
        const auto forward
            = Distance::computeEpicentralDistance(hypocenter(39.6, -111.4),
                                                  station(40.76, -111.85));
        const auto backward
            = Distance::computeEpicentralDistance(hypocenter(40.76, -111.85),
                                                  station(39.6, -111.4));
        REQUIRE(forward > 100000);
        REQUIRE(forward < 150000);
        REQUIRE_THAT(backward, Catch::Matchers::WithinAbs(forward, 1.e-6));
    }

    SECTION("Longitude conventions don't matter")
    {
        const auto west
            = Distance::computeEpicentralDistance(hypocenter(39.6, -111.4),
                                                  station(40.76, -111.85));
        const auto east
            = Distance::computeEpicentralDistance(hypocenter(39.6, 248.6),
                                                  station(40.76, 248.15));
        REQUIRE_THAT(east, Catch::Matchers::WithinAbs(west, 1.e-6));
        // The short way across the antimeridian
        REQUIRE_THAT(Distance::computeEpicentralDistance(hypocenter(0, 179.9),
                                                         station(0, -179.9)),
                     Catch::Matchers::WithinAbs(0.2*111319.49079327358,
                                                1.e-6));
    }

    SECTION("Depth doesn't change an epicentral distance")
    {
        auto deep = hypocenter(39.6, -111.4);
        deep.setDepth(20000);
        REQUIRE(Distance::computeEpicentralDistance(deep,
                                                    station(40.76, -111.85))
             == Distance::computeEpicentralDistance(hypocenter(39.6, -111.4),
                                                    station(40.76, -111.85)));
    }

    SECTION("Latitude and longitude are required")
    {
        const auto good = hypocenter(39.6, -111.4);
        const auto goodStation = station(40.76, -111.85);
        Hypocenter noLatitude;
        noLatitude.setLongitude(-111.4);
        Hypocenter noLongitude;
        noLongitude.setLatitude(39.6);
        StationLocation noStationLatitude;
        noStationLatitude.setLongitude(-111.85);
        StationLocation noStationLongitude;
        noStationLongitude.setLatitude(40.76);
        REQUIRE_THROWS_AS(
            Distance::computeEpicentralDistance(noLatitude, goodStation),
            std::invalid_argument);
        REQUIRE_THROWS_AS(
            Distance::computeEpicentralDistance(noLongitude, goodStation),
            std::invalid_argument);
        REQUIRE_THROWS_AS(
            Distance::computeEpicentralDistance(good, noStationLatitude),
            std::invalid_argument);
        REQUIRE_THROWS_AS(
            Distance::computeEpicentralDistance(good, noStationLongitude),
            std::invalid_argument);
        REQUIRE_THROWS_AS(
            Distance::computeEpicentralDistance(Hypocenter {}, StationLocation {}),
            std::invalid_argument);
    }

    SECTION("Feeds the distance correction")
    {
        // Roughly 130 km - well inside the 600 km Utah table
        const auto utah = fromUUSSTable("Utah");
        const auto distance
            = Distance::computeEpicentralDistance(hypocenter(39.6, -111.4),
                                                  station(40.76, -111.85));
        REQUIRE(utah(distance).has_value());
    }
}

TEST_CASE("ULocalMagnitudeService::Corrections::DistanceOptions - datum",
          "[distanceOptions]")
{
    SECTION("Defaults to sea level")
    {
        REQUIRE(DistanceOptions {}.getDatum() == 0);
        REQUIRE(uussOptions("Utah").getDatum() == 0);
        // The test copy of the Yellowstone table matches what AQMS computes,
        // and AQMS doesn't apply the datum
        REQUIRE(uussOptions("Yellowstone").getDatum() == 0);
    }

    SECTION("Range")
    {
        DistanceOptions options;
        for (const double datum : {-10000.0, -150.0, 0.0, 2000.0, 8600.0})
        {
            options.setDatum(datum);
            REQUIRE(options.getDatum() == datum);
        }
        for (const double datum :
                 {-10000.1, 8600.1,
                  std::numeric_limits<double>::quiet_NaN(),
                  std::numeric_limits<double>::infinity(),
                  -std::numeric_limits<double>::infinity()})
        {
            INFO("Datum: " << datum);
            REQUIRE_THROWS_AS(options.setDatum(datum), std::invalid_argument);
        }
        // A rejected datum preserves the previous one
        REQUIRE(options.getDatum() == 8600);
    }

    SECTION("Copy and move")
    {
        DistanceOptions options;
        options.setDatum(2000);
        const DistanceOptions copy{options};
        REQUIRE(copy.getDatum() == 2000);
        options.setDatum(0);
        REQUIRE(copy.getDatum() == 2000);
        const DistanceOptions moved{std::move(options)};
        REQUIRE(moved.getDatum() == 0);
    }

    SECTION("From an initialization file")
    {
        const TemporaryIniFile withDatum("datum",
                                         "[DistanceCorrections]\n"
                                         "interpolation = linear\n"
                                         "distanceType = hypocentral\n"
                                         "datum = 2000\n"
                                         "distance_correction_1 = 0, 1.0\n");
        REQUIRE(DistanceOptions::fromInitializationFile(withDatum.path())
                .getDatum() == 2000);

        const TemporaryIniFile withoutDatum("noDatum",
                                            "[DistanceCorrections]\n"
                                            "interpolation = linear\n"
                                            "distanceType = hypocentral\n"
                                            "distance_correction_1 = 0, 1.0\n");
        REQUIRE(DistanceOptions::fromInitializationFile(withoutDatum.path())
                .getDatum() == 0);

        // An epicentral distance has no depth to shift so the datum is
        // ignored
        const TemporaryIniFile epicentral("epicentralDatum",
                                          "[DistanceCorrections]\n"
                                          "interpolation = linear\n"
                                          "distanceType = epicentral\n"
                                          "datum = 2000\n"
                                          "distance_correction_1 = 0, 1.0\n");
        REQUIRE(DistanceOptions::fromInitializationFile(epicentral.path())
                .getDatum() == 0);

        const TemporaryIniFile badDatum("badDatum",
                                        "[DistanceCorrections]\n"
                                        "interpolation = linear\n"
                                        "distanceType = hypocentral\n"
                                        "datum = 9000\n"
                                        "distance_correction_1 = 0, 1.0\n");
        REQUIRE_THROWS_AS(
            DistanceOptions::fromInitializationFile(badDatum.path()),
            std::invalid_argument);
    }
}

TEST_CASE("ULocalMagnitudeService::Corrections::Distance - datum",
          "[distance]")
{
    // Linear on {0, 1}, {10, 2}, {30, 4} so a 5 m hypocentral distance gives
    // 1.5.  The hypocentral distance uses depth + datum - i.e., the depth
    // below a datum that is positive up.
    const auto makeDistance = [](const DistanceOptions::Type type,
                                 const double datum)
    {
        DistanceOptions options;
        options.setCorrections({{0, 1.0}, {10, 2.0}, {30, 4.0}});
        options.setInterpolation(DistanceOptions::Interpolation::Linear);
        options.setType(type);
        options.setDatum(datum);
        return Distance {options};
    };

    SECTION("The datum shifts the depth")
    {
        const auto distance
            = makeDistance(DistanceOptions::Type::Hypocentral, 2);
        // 2 m deep is 4 m below a datum 2 m above sea level: 3-4-5
        REQUIRE_THAT(distance(3, 2).value(),
                     Catch::Matchers::WithinAbs(1.5, 1.e-12));
        // Without the datum the same event is closer
        REQUIRE_THAT(makeDistance(DistanceOptions::Type::Hypocentral, 0)
                         (3, 2).value(),
                     Catch::Matchers::WithinAbs(
                         1 + 0.1*std::hypot(3.0, 2.0), 1.e-12));
    }

    SECTION("Above the datum is the same as below it")
    {
        const auto distance
            = makeDistance(DistanceOptions::Type::Hypocentral, 2);
        // 6 m above sea level is 4 m above the datum
        REQUIRE_THAT(distance(3, -6).value(),
                     Catch::Matchers::WithinAbs(1.5, 1.e-12));
        // At the datum the hypocentral distance is the epicentral distance
        REQUIRE_THAT(distance(5, -2).value(),
                     Catch::Matchers::WithinAbs(1.5, 1.e-12));
    }

    SECTION("A negative datum")
    {
        // 6 m deep is 4 m below a datum 2 m below sea level
        const auto distance
            = makeDistance(DistanceOptions::Type::Hypocentral, -2);
        REQUIRE_THAT(distance(3, 6).value(),
                     Catch::Matchers::WithinAbs(1.5, 1.e-12));
    }

    SECTION("The datum can push a station past the table")
    {
        // 24 m deep is 29.4 m away - just inside the 30 m table - but
        // 30 m below a datum 6 m above sea level is 34.5 m away
        const auto distance
            = makeDistance(DistanceOptions::Type::Hypocentral, 6);
        REQUIRE(makeDistance(DistanceOptions::Type::Hypocentral, 0)
                    (17, 24).has_value());
        const auto tooFar = distance(17, 24);
        REQUIRE_FALSE(tooFar.has_value());
        REQUIRE(tooFar.error() == Distance::ErrorCode::StationTooFar);
    }

    SECTION("The depth range applies to the catalog depth")
    {
        const auto distance
            = makeDistance(DistanceOptions::Type::Hypocentral, 8600);
        // -8600 relative to sea level is valid and sits right at a datum
        // 8600 m above sea level
        REQUIRE_THAT(distance(0, -8600).value(),
                     Catch::Matchers::WithinAbs(1.0, 1.e-12));
        REQUIRE(distance(0, -8601).error()
             == Distance::ErrorCode::InvalidSourceDepth);
    }

    SECTION("An epicentral distance ignores the datum")
    {
        const auto distance
            = makeDistance(DistanceOptions::Type::Epicentral, 2000);
        REQUIRE_THAT(distance(5, 7000).value(),
                     Catch::Matchers::WithinAbs(1.5, 1.e-12));
    }

    SECTION("Yellowstone with the Holt et al. 2 km datum")
    {
        auto options = uussOptions("Yellowstone");
        const Distance aqms{options};
        options.setDatum(2000);
        const Distance withDatum{options};
        // Orid 91358 was 5.14 km below sea level so it is 7.14 km below the
        // model's datum
        constexpr double epicentralDistance{38806.67814609038};
        REQUIRE_THAT(withDatum(epicentralDistance, 5140).value(),
                     Catch::Matchers::WithinAbs(
                         aqms(std::hypot(epicentralDistance, 7140.0)).value(),
                         1.e-12));
        // Deeper is further and for Yellowstone that's a bigger correction
        REQUIRE(withDatum(epicentralDistance, 5140).value()
              > aqms(epicentralDistance, 5140).value());
    }
}
