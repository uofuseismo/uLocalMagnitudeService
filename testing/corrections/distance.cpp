#include <cstddef>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitude/corrections/distance.hpp"
#include "uLocalMagnitude/corrections/distanceOptions.hpp"

using namespace ULocalMagnitude::Corrections;

namespace
{
/// Writes an initialization file to the temporary directory and removes it
/// when it goes out of scope.
class TemporaryIniFile
{
public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    TemporaryIniFile(const std::string &name, const std::string &contents) :
        mPath(std::filesystem::temp_directory_path()
            / ("uLocalMagnitude_" + name + ".ini"))
    {
        std::ofstream file(mPath);
        file << contents;
    }
    ~TemporaryIniFile()
    {
        std::error_code errorCode;
        std::filesystem::remove(mPath, errorCode);
    }
    TemporaryIniFile(const TemporaryIniFile &) = delete;
    TemporaryIniFile& operator=(const TemporaryIniFile &) = delete;
    [[nodiscard]] const std::filesystem::path &path() const noexcept
    {
        return mPath;
    }
private:
    std::filesystem::path mPath;
};

/// Creates a distance correction from one of the hardwired tables.
Distance fromHardwiredTable(const std::string &table,
                            const std::string &interpolation)
{
    const TemporaryIniFile iniFile(table + "_" + interpolation,
                                   "[DistanceCorrections]\n"
                                   "interpolation = " + interpolation + "\n"
                                   "use" + table + "Corrections = true\n");
    return Distance {fromInitializationFile(iniFile.path())};
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

TEST_CASE("ULocalMagnitude::Corrections::DistanceOptions", "[distanceOptions]")
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
        REQUIRE(options.getInterpolation() ==
                DistanceOptions::Interpolation::Nearest);
        REQUIRE_THROWS_AS(options.getCorrections(), std::runtime_error);
        REQUIRE_THROWS_AS(options.getCorrectionsReference(),
                          std::runtime_error);
    }

    SECTION("Interpolation")
    {
        DistanceOptions options;
        options.setInterpolation(DistanceOptions::Interpolation::Linear);
        REQUIRE(options.getInterpolation() ==
                DistanceOptions::Interpolation::Linear);
        options.setInterpolation(DistanceOptions::Interpolation::Nearest);
        REQUIRE(options.getInterpolation() ==
                DistanceOptions::Interpolation::Nearest);
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

TEST_CASE("ULocalMagnitude::Corrections::fromInitializationFile",
          "[distanceOptions]")
{
    SECTION("Missing file")
    {
        REQUIRE_THROWS_AS(
            fromInitializationFile("/this/file/does/not/exist.ini"),
            std::invalid_argument);
    }

    SECTION("Hardwired tables")
    {
        const TemporaryIniFile utahFile("utahTable",
                                        "[DistanceCorrections]\n"
                                        "interpolation = nearest\n"
                                        "useUtahCorrections = true\n");
        const auto utah = fromInitializationFile(utahFile.path());
        REQUIRE(utah.getInterpolation() ==
                DistanceOptions::Interpolation::Nearest);
        const auto &utahTable = utah.getCorrectionsReference();
        REQUIRE(utahTable.size() == 70);
        REQUIRE(utahTable.front() == std::pair {0.0, 1.4});
        REQUIRE(utahTable.back() == std::pair {600000.0, 4.9});

        const TemporaryIniFile yellowstoneFile("yellowstoneTable",
                                               "[DistanceCorrections]\n"
                                               "interpolation = nearest\n"
                                               "useYellowstoneCorrections = true\n");
        const auto yellowstone = fromInitializationFile(yellowstoneFile.path());
        const auto &yellowstoneTable = yellowstone.getCorrectionsReference();
        REQUIRE(yellowstoneTable.size() == 39);
        REQUIRE(yellowstoneTable.front() == std::pair {3000.0, 0.64});
        REQUIRE(yellowstoneTable.back() == std::pair {180000.0, 3.67});
    }

    SECTION("Interpolation is case insensitive")
    {
        const TemporaryIniFile iniFile("caseInsensitive",
                                       "[DistanceCorrections]\n"
                                       "interpolation = LiNeAr\n"
                                       "useUtahCorrections = true\n");
        REQUIRE(fromInitializationFile(iniFile.path()).getInterpolation() ==
                DistanceOptions::Interpolation::Linear);
    }

    SECTION("Custom table in a custom section")
    {
        // Mixes the separators a user might reasonably type and is out of
        // order.
        const TemporaryIniFile iniFile("customTable",
                                       "[MyCorrections]\n"
                                       "interpolation = linear\n"
                                       "distance_correction_1 = 10000, 2.0\n"
                                       "distance_correction_2 =   0    1.0\n"
                                       "distance_correction_3 = 30000\t4.0\n"
                                       "distance_correction_4 = 20000 , 3.0\n");
        const auto options
            = fromInitializationFile(iniFile.path(), "MyCorrections");
        REQUIRE(options.getInterpolation() ==
                DistanceOptions::Interpolation::Linear);
        checkCorrections({{0, 1.0}, {10000, 2.0}, {20000, 3.0}, {30000, 4.0}},
                         options.getCorrections());
    }

    SECTION("Invalid interpolation")
    {
        const TemporaryIniFile iniFile("badInterpolation",
                                       "[DistanceCorrections]\n"
                                       "interpolation = cubic\n"
                                       "useUtahCorrections = true\n");
        REQUIRE_THROWS_AS(fromInitializationFile(iniFile.path()),
                          std::invalid_argument);
    }

    SECTION("Invalid table entry")
    {
        const TemporaryIniFile iniFile("badEntry",
                                       "[DistanceCorrections]\n"
                                       "interpolation = nearest\n"
                                       "distance_correction_1 = 0, 1.0\n"
                                       "distance_correction_2 = 1 2 3\n");
        REQUIRE_THROWS_AS(fromInitializationFile(iniFile.path()),
                          std::invalid_argument);
    }

    SECTION("No table")
    {
        const TemporaryIniFile iniFile("noTable",
                                       "[DistanceCorrections]\n"
                                       "interpolation = nearest\n");
        REQUIRE_THROWS_AS(fromInitializationFile(iniFile.path()),
                          std::invalid_argument);
    }
}

TEST_CASE("ULocalMagnitude::Corrections::Distance", "[distance]")
{
    // Distances in meters
    DistanceOptions options;
    options.setCorrections({{0, 1.0}, {10, 2.0}, {30, 4.0}});

    SECTION("Requires corrections")
    {
        REQUIRE_THROWS_AS(Distance {DistanceOptions {}}, std::runtime_error);
    }

    SECTION("Invalid distances")
    {
        const Distance distance{options};
        REQUIRE_THROWS_AS(distance(-1.0), std::invalid_argument);
        REQUIRE_NOTHROW(distance(21000000.0));
        REQUIRE_THROWS_AS(distance(21000001.0), std::invalid_argument);
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
        REQUIRE(distance(1000) == 4.0);
    }

    SECTION("Linear")
    {
        options.setInterpolation(DistanceOptions::Interpolation::Linear);
        const Distance distance{options};
        REQUIRE_THAT(distance(0),    Catch::Matchers::WithinAbs(1.0, 1.e-12));
        REQUIRE_THAT(distance(5),    Catch::Matchers::WithinAbs(1.5, 1.e-12));
        REQUIRE_THAT(distance(10),   Catch::Matchers::WithinAbs(2.0, 1.e-12));
        REQUIRE_THAT(distance(20),   Catch::Matchers::WithinAbs(3.0, 1.e-12));
        REQUIRE_THAT(distance(25),   Catch::Matchers::WithinAbs(3.5, 1.e-12));
        REQUIRE_THAT(distance(30),   Catch::Matchers::WithinAbs(4.0, 1.e-12));
        REQUIRE_THAT(distance(1000), Catch::Matchers::WithinAbs(4.0, 1.e-12));
    }

    SECTION("Linear saturates below the first node")
    {
        DistanceOptions shifted;
        shifted.setInterpolation(DistanceOptions::Interpolation::Linear);
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
            single.setCorrections({{5000, 0.25}});
            const Distance distance{single};
            REQUIRE(distance(0) == 0.25);
            REQUIRE(distance(5000) == 0.25);
            REQUIRE(distance(100000) == 0.25);
        }
    }

    SECTION("Copy and move")
    {
        options.setInterpolation(DistanceOptions::Interpolation::Linear);
        const Distance distance{options};
        // Exercising the copy constructor is the point
        // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
        const Distance copy{distance};
        REQUIRE_THAT(copy(5), Catch::Matchers::WithinAbs(1.5, 1.e-12));
        Distance copyAssigned{options};
        copyAssigned = copy;
        REQUIRE_THAT(copyAssigned(5), Catch::Matchers::WithinAbs(1.5, 1.e-12));
        Distance moved{std::move(copyAssigned)};
        REQUIRE_THAT(moved(5), Catch::Matchers::WithinAbs(1.5, 1.e-12));
        Distance moveAssigned{options};
        moveAssigned = std::move(moved);
        REQUIRE_THAT(moveAssigned(5), Catch::Matchers::WithinAbs(1.5, 1.e-12));
        checkCorrections({{0, 1.0}, {10, 2.0}, {30, 4.0}},
                         moveAssigned.getCorrections());
    }

    SECTION("Corrections table")
    {
        DistanceOptions unsorted;
        unsorted.setCorrections({{30, 4.0}, {0, 1.0}, {10, 2.0}});
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

TEST_CASE("ULocalMagnitude::Corrections::Distance - Utah and Yellowstone",
          "[distance]")
{
    // The lookup takes meters but the tables are written in kilometers.
    const auto utahNearest = fromHardwiredTable("Utah", "nearest");
    const auto yellowstoneNearest = fromHardwiredTable("Yellowstone", "nearest");
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
        REQUIRE(atKm(yellowstoneNearest, 3.0)   == 0.64);
        REQUIRE(atKm(yellowstoneNearest, 180.0) == 3.67);
    }

    SECTION("Between two entries the nearest one applies")
    {
        REQUIRE(atKm(utahNearest, 31.0)  == 2.1);  // 30 -> 35
        REQUIRE(atKm(utahNearest, 32.5)  == 2.1);  // Tie goes to 30
        REQUIRE(atKm(utahNearest, 32.6)  == 2.3);
        REQUIRE(atKm(utahNearest, 34.99) == 2.3);
        REQUIRE(atKm(utahNearest, 35.0)  == 2.3);
        REQUIRE(atKm(yellowstoneNearest, 4.0) == 0.64); // 3 -> 6
        REQUIRE(atKm(yellowstoneNearest, 4.5) == 0.64); // Tie goes to 3
        REQUIRE(atKm(yellowstoneNearest, 5.0) == 0.72);
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
        REQUIRE(atKm(yellowstoneNearest, 0.0) == 0.64);
        REQUIRE(atKm(yellowstoneNearest, 2.9) == 0.64);
    }

    SECTION("Further than the table goes takes the last correction")
    {
        REQUIRE(atKm(utahNearest, 601.0)  == 4.9);
        REQUIRE(atKm(utahNearest, 5000.0) == 4.9);
        REQUIRE(atKm(yellowstoneNearest, 181.0)  == 3.67);
        REQUIRE(atKm(yellowstoneNearest, 5000.0) == 3.67);
    }

    SECTION("The two regions are different curves, not an offset")
    {
        REQUIRE(atKm(utahNearest, 50.0) == 2.6);
        REQUIRE(atKm(yellowstoneNearest, 50.0) == 2.69);
        // Utah is the higher curve close in and the lower one far out -
        // they cross so no single offset relates them.
        REQUIRE(atKm(utahNearest, 15.0) == 1.6);
        REQUIRE(atKm(yellowstoneNearest, 15.0) == 1.33);
        REQUIRE(atKm(utahNearest, 150.0) == 3.3);
        REQUIRE(atKm(yellowstoneNearest, 150.0) == 3.5);
    }

    SECTION("The Yellowstone dip is preserved, not smoothed")
    {
        // The corrections rise to 3.17 at 80 km, fall to 3.06 at 110, then
        // rise again.  That is in the source table and a "fix" that made
        // it monotonic would be wrong.
        REQUIRE(atKm(yellowstoneNearest, 80.0)  == 3.17);
        REQUIRE(atKm(yellowstoneNearest, 110.0) == 3.06);
        REQUIRE(atKm(yellowstoneNearest, 140.0) == 3.37);
        REQUIRE(atKm(yellowstoneNearest, 110.0)
              < atKm(yellowstoneNearest, 80.0));
    }

    SECTION("Linear interpolation")
    {
        const auto utahLinear = fromHardwiredTable("Utah", "linear");
        const auto yellowstoneLinear
            = fromHardwiredTable("Yellowstone", "linear");
        // Nodes are unchanged
        REQUIRE_THAT(atKm(utahLinear, 30.0),
                     Catch::Matchers::WithinAbs(2.1, 1.e-12));
        // Midpoints
        REQUIRE_THAT(atKm(utahLinear, 32.5),
                     Catch::Matchers::WithinAbs(2.2, 1.e-12));
        REQUIRE_THAT(atKm(utahLinear, 72.5),
                     Catch::Matchers::WithinAbs(2.825, 1.e-12));
        REQUIRE_THAT(atKm(yellowstoneLinear, 4.5),
                     Catch::Matchers::WithinAbs(0.68, 1.e-12));
        // Saturates at the extrema
        REQUIRE_THAT(atKm(yellowstoneLinear, 0.0),
                     Catch::Matchers::WithinAbs(0.64, 1.e-12));
        REQUIRE_THAT(atKm(utahLinear, 700.0),
                     Catch::Matchers::WithinAbs(4.9, 1.e-12));
    }

    SECTION("Negative distances are rejected")
    {
        REQUIRE_THROWS_AS(utahNearest(-1.0), std::invalid_argument);
        REQUIRE_NOTHROW(utahNearest(0.0));
    }
}
