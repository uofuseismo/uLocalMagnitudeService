#include <stdexcept>
#include <string>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitudeService/corrections/station.hpp"
#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"
#include "uLocalMagnitudeService/corrections/stationOptions.hpp"
#include "uLocalMagnitudeService/corrections/stationsSet.hpp"
#include "../distanceTables.hpp"

using namespace ULocalMagnitudeService::Corrections;
using ULocalMagnitudeService::Testing::TemporaryIniFile;

namespace
{
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void checkCorrection(const StationsSet &set,
                     const std::string &name,
                     const double expected)
{
    const auto correction = set.getCorrection(name);
    REQUIRE(correction.has_value());
    REQUIRE(correction->getName() == name);
    REQUIRE_THAT((*correction)(),
                 Catch::Matchers::WithinAbs(expected, 1.e-14));
}

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

TEST_CASE("ULocalMagnitudeService::Corrections::StationsSet", "[stationsSet]")
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

TEST_CASE("ULocalMagnitudeService::Corrections::StationsSet::fromInitializationFile",
          "[stationsSet]")
{
    SECTION("Missing file")
    {
        REQUIRE_THROWS_AS(
            StationsSet::fromInitializationFile("/this/file/does/not/exist.ini"),
            std::invalid_argument);
    }

    SECTION("Documented format")
    {
        const TemporaryIniFile iniFile("stationsDocumented",
                                       "[StationCorrections]\n"
                                       "station_correction_1 = UU CWU,  0.4\n"
                                       "station_correction_2 = UU KNB, -0.35\n");
        const auto set = StationsSet::fromInitializationFile(iniFile.path());
        REQUIRE(set.getCorrections().size() == 2);
        checkCorrection(set, "UU.CWU", 0.4);
        checkCorrection(set, "UU.KNB", -0.35);
    }

    SECTION("Separators a user might reasonably type")
    {
        const TemporaryIniFile iniFile("stationsSeparators",
                                       "[StationCorrections]\n"
                                       "station_correction_1 = UU.CWU, 0.4\n"
                                       "station_correction_2 = WY.YFT 0.18\n"
                                       "station_correction_3 = WY  YMR\t-0.1\n"
                                       "station_correction_4 = US,BOZ,0.17\n"
                                       "station_correction_5 = uu ctu, 0\n"
                                       "station_correction_6 = UU B206, 1\n"
                                       "station_correction_7 = UU SRU, .25\n");
        const auto set = StationsSet::fromInitializationFile(iniFile.path());
        REQUIRE(set.getCorrections().size() == 7);
        checkCorrection(set, "UU.CWU", 0.4);
        checkCorrection(set, "WY.YFT", 0.18);
        checkCorrection(set, "WY.YMR", -0.1);
        checkCorrection(set, "US.BOZ", 0.17);
        // Names are capitalized
        checkCorrection(set, "UU.CTU", 0);
        checkCorrection(set, "UU.B206", 1);
        checkCorrection(set, "UU.SRU", 0.25);
    }

    SECTION("Custom section")
    {
        const TemporaryIniFile iniFile("stationsCustomSection",
                                       "[StationCorrections]\n"
                                       "station_correction_1 = UU CWU, 0.4\n"
                                       "[Yellowstone]\n"
                                       "station_correction_1 = WY YFT, 0.18\n"
                                       "station_correction_2 = WY YMR, -0.1\n");
        const auto set
            = StationsSet::fromInitializationFile(iniFile.path(), "Yellowstone");
        REQUIRE(set.getCorrections().size() == 2);
        checkCorrection(set, "WY.YFT", 0.18);
        checkCorrection(set, "WY.YMR", -0.1);
        REQUIRE_FALSE(set.getCorrection("UU.CWU").has_value());
    }

    SECTION("No corrections")
    {
        const TemporaryIniFile iniFile("stationsNone", "[StationCorrections]\n");
        REQUIRE(StationsSet::fromInitializationFile(iniFile.path()).empty());
    }

    SECTION("Parsing stops at the first gap")
    {
        const TemporaryIniFile iniFile("stationsGap",
                                       "[StationCorrections]\n"
                                       "station_correction_1 = UU CWU, 0.4\n"
                                       "station_correction_2 = UU KNB, -0.35\n"
                                       "station_correction_4 = WY YFT, 0.18\n");
        const auto set = StationsSet::fromInitializationFile(iniFile.path());
        REQUIRE(set.getCorrections().size() == 2);
        REQUIRE_FALSE(set.getCorrection("WY.YFT").has_value());
    }

    SECTION("Duplicates are rejected")
    {
        const TemporaryIniFile duplicate("stationsDuplicate",
                                         "[StationCorrections]\n"
                                         "station_correction_1 = UU CWU, 0.4\n"
                                         "station_correction_2 = UU CWU, 0.3\n");
        REQUIRE_THROWS_AS(StationsSet::fromInitializationFile(duplicate.path()),
                          std::invalid_argument);
        // Case and separators don't make it a different station
        const TemporaryIniFile disguised("stationsDisguised",
                                         "[StationCorrections]\n"
                                         "station_correction_1 = UU CWU, 0.4\n"
                                         "station_correction_2 = uu.cwu 0.3\n");
        REQUIRE_THROWS_AS(StationsSet::fromInitializationFile(disguised.path()),
                          std::invalid_argument);
    }

    SECTION("Invalid entries are rejected")
    {
        for (const std::string entry : {"UU, 0.4",          // No station
                                        "UU CWU",           // No correction
                                        "UU CWU 0.4 0.5",   // Too many fields
                                        "UU.CWU.01, 0.4",   // Not a station
                                        "UU CWU, abc"})     // Not a number
        {
            INFO("Entry: " << entry);
            const TemporaryIniFile iniFile("stationsInvalid",
                                           "[StationCorrections]\n"
                                           "station_correction_1 = UU KNB, 0.1\n"
                                           "station_correction_2 = "
                                         + entry + "\n");
            REQUIRE_THROWS_AS(StationsSet::fromInitializationFile(iniFile.path()),
                              std::invalid_argument);
        }
    }
}
