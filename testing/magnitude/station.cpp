#include <array>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitudeService/magnitude/station.hpp"
#include "uLocalMagnitudeService/magnitude/amplitude.hpp"
#include "uLocalMagnitudeService/magnitude/streamIdentifier.hpp"
#include "uLocalMagnitudeService/corrections/distance.hpp"
#include "uLocalMagnitudeService/corrections/distanceOptions.hpp"
#include "uLocalMagnitudeService/corrections/station.hpp"
#include "uLocalMagnitudeService/corrections/stationIdentifier.hpp"
#include "uLocalMagnitudeService/corrections/stationOptions.hpp"
#include "../distanceTables.hpp"

using namespace ULocalMagnitudeService;

namespace
{
/// The Utah distance corrections with nearest interpolation on the
/// epicentral distance - this is what AQMS uses.
Corrections::Distance utahDistanceCorrections()
{
    const Testing::TemporaryIniFile iniFile("magnitudeStationUtah",
                                            Testing::utahIniSection());
    return Corrections::Distance {
        Corrections::fromInitializationFile(iniFile.path())};
}

/// The Yellowstone distance corrections with linear interpolation on the
/// hypocentral distance - this is what AQMS uses.
Corrections::Distance yellowstoneDistanceCorrections()
{
    const Testing::TemporaryIniFile iniFile("magnitudeStationYellowstone",
                                            Testing::yellowstoneIniSection());
    return Corrections::Distance {
        Corrections::fromInitializationFile(iniFile.path())};
}

Corrections::Station stationCorrection(const std::string &network,
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

Magnitude::Amplitude amplitude(const std::string &network,
                               const std::string &station,
                               const std::string &channel,
                               const std::string &locationCode,
                               const double value)
{
    Magnitude::StreamIdentifier identifier;
    identifier.setNetwork(network);
    identifier.setStation(station);
    identifier.setChannel(channel);
    identifier.setLocationCode(locationCode);
    Magnitude::Amplitude result;
    result.setIdentifier(identifier);
    result.setValue(value);
    return result;
}

/// An observation from testing/data/amp-ml-uu80157466.csv - Ml 2.04 for
/// evid 80157466 (magid 121968).  AQMS stores the amplitudes in cm.
struct Observation
{
    const char *station;
    double amplitudeEastInCentimeters;
    double amplitudeNorthInCentimeters;
    double stationCorrection;
    double distanceInMeters;
    double distanceCorrection;
    double stationMagnitude;
};

constexpr std::array<Observation, 6> observations
{{
    {"CCUT", 0.03459206596016884, 0.015187045093625784,  0.31,
     66775.14936328208,  2.8, 2.204987145462916},
    {"LCMT", 0.2695453315973282,  0.3341444283723831,   -0.12,
     31787.222423682797, 2.1, 2.158753817909035},
    {"PKCU", 0.032379960641264915, 0.023217148147523403, -0.32,
     65753.37585962987,  2.8, 1.62299221626627},
    {"SZCU", 0.11622115224599838, 0.12265413627028465,  -0.1,
     59897.98846614931,  2.8, 2.476111233281225},
    {"VRUT", 0.012941689230501652, 0.00795073457993567,  0.1,
     95134.13961579288,  3.0, 1.8179288357483208},
    {"ZNPU", 0.12780731543898582, 0.16992953419685364,  -0.2,
     36945.35548051267,  2.3, 1.9717725974441087}
}};

/// An observation from testing/data/amp-ml-uu80157536.csv - Ml 1.58 for
/// evid 80157536 (magid 121938) in Yellowstone.  AQMS stores the amplitudes
/// in cm and the distances are epicentral.
struct YellowstoneObservation
{
    const char *network;
    const char *station;
    const char *channel1;
    const char *channel2;
    const char *locationCode;
    double amplitude1InCentimeters;
    double amplitude2InCentimeters;
    double stationCorrection;
    double epicentralDistanceInMeters;
    double stationMagnitude;
};

constexpr std::array<YellowstoneObservation, 8> yellowstoneObservations
{{
    {"US", "BOZ", "BH1", "BH2", "00", 0.012647671159356833,
     0.01141107827425003,   0.17, 107589.10395903757, 2.0137894296247505},
    {"WY", "YFT", "HHE", "HHN", "01", 0.017387771047651768,
     0.01901680789887905,   0.18, 38806.67814609038,  1.5151727743443948},
    {"WY", "YHB", "HHE", "HHN", "01", 0.06612673029303551,
     0.13392416387796402,   0.06, 27821.31522658042,  1.8110132626551372},
    {"WY", "YHL", "HHE", "HHN", "01", 0.039701126515865326,
     0.03223342541605234,   0.21, 26809.956748755743, 1.483017383766168},
    {"WY", "YMR", "HHE", "HHN", "01", 0.07399860955774784,
     0.062198705971241,    -0.1,  17186.024815819168, 0.9775766170886758},
    {"WY", "YNE", "HHE", "HHN", "01", 0.005994296399876475,
     0.007737071951851249, -0.14, 70260.72126291043,  1.5092416785525842},
    {"WY", "YNR", "HHE", "HHN", "01", 0.3462141752243042,
     0.18777716904878616,   0.06, 16492.250327782884, 1.6822827168241627},
    {"WY", "YUF", "HHE", "HHN", "01", 0.08320801332592964,
     0.06868967227637768,   0,    28481.028778183314, 1.6534893472932934}
}};

/// Utah uses epicentral distance so the depth shouldn't matter.  This is
/// deliberately large enough to change the Utah correction if it were used.
constexpr double depth{15000};

/// Source depth of orid 91358.
constexpr double yellowstoneDepthInMeters{5140};

constexpr double centimetersToMillimeters{10};
}

TEST_CASE("ULocalMagnitudeService::Magnitude::Station - AQMS",
          "[magnitudeStation]")
{
    const auto distance = utahDistanceCorrections();
    for (const auto &observation : observations)
    {
        const std::string name{observation.station};
        INFO("Station: " << name);
        const Magnitude::Station station{
            stationCorrection("UU", name, observation.stationCorrection),
            distance};
        REQUIRE(station.isInitialized());
        REQUIRE_THAT(station.getStationCorrection(),
                     Catch::Matchers::WithinAbs(observation.stationCorrection,
                                                1.e-14));
        REQUIRE_THAT(station.getDistanceCorrection(observation.distanceInMeters, depth),
                     Catch::Matchers::WithinAbs(observation.distanceCorrection,
                                                1.e-14));
        const auto east
            = amplitude("UU", name, "HHE", "01",
                        centimetersToMillimeters
                       *observation.amplitudeEastInCentimeters);
        const auto north
            = amplitude("UU", name, "HHN", "01",
                        centimetersToMillimeters
                       *observation.amplitudeNorthInCentimeters);
        const auto magnitude
            = station(std::pair {east, north}, observation.distanceInMeters, depth);
        REQUIRE_THAT(magnitude,
                     Catch::Matchers::WithinAbs(observation.stationMagnitude,
                                                1.e-10));
        // Order of the channels doesn't matter
        const auto magnitudeReversed
            = station(std::pair {north, east}, observation.distanceInMeters, depth);
        REQUIRE_THAT(magnitudeReversed,
                     Catch::Matchers::WithinAbs(magnitude, 1.e-14));
    }
}

TEST_CASE("ULocalMagnitudeService::Magnitude::Station - AQMS Yellowstone",
          "[magnitudeStation]")
{
    const auto distance = yellowstoneDistanceCorrections();
    for (const auto &observation : yellowstoneObservations)
    {
        const std::string network{observation.network};
        const std::string name{observation.station};
        INFO("Station: " << network << "." << name);
        const Magnitude::Station station{
            stationCorrection(network, name, observation.stationCorrection),
            distance};
        const auto amplitude1
            = amplitude(network, name, observation.channel1,
                        observation.locationCode,
                        centimetersToMillimeters
                       *observation.amplitude1InCentimeters);
        const auto amplitude2
            = amplitude(network, name, observation.channel2,
                        observation.locationCode,
                        centimetersToMillimeters
                       *observation.amplitude2InCentimeters);
        // AQMS interpolates the Yellowstone table at the hypocentral distance
        const auto magnitude
            = station(std::pair {amplitude1, amplitude2},
                      observation.epicentralDistanceInMeters,
                      yellowstoneDepthInMeters);
        REQUIRE_THAT(magnitude,
                     Catch::Matchers::WithinAbs(observation.stationMagnitude,
                                                1.e-10));
    }
}

TEST_CASE("ULocalMagnitudeService::Magnitude::Station",
          "[magnitudeStation]")
{
    const auto distance = utahDistanceCorrections();
    const Magnitude::Station station{stationCorrection("UU", "CCUT", 0.31),
                                     distance};
    const auto east = amplitude("UU", "CCUT", "HHE", "01", 0.3459206596016884);
    const auto north = amplitude("UU", "CCUT", "HHN", "01", 0.15187045093625784);
    constexpr double distanceInMeters{66775.14936328208};
    constexpr double expectedMagnitude{2.204987145462916};

    SECTION("Formula")
    {
        // log10(AverageAmplitude/2) + C_d + C_s
        const auto east1 = amplitude("UU", "CCUT", "HHE", "01", 3);
        const auto north1 = amplitude("UU", "CCUT", "HHN", "01", 1);
        // log10(0.5*0.5*(3 + 1)) = 0
        REQUIRE_THAT(station(std::pair {east1, north1}, 0, depth),
                     Catch::Matchers::WithinAbs(1.4 + 0.31, 1.e-14));
        REQUIRE_THAT(station(std::pair {east1, north1}, 100000, depth),
                     Catch::Matchers::WithinAbs(3.0 + 0.31, 1.e-14));
        const auto east2 = amplitude("UU", "CCUT", "HHE", "01", 20);
        const auto north2 = amplitude("UU", "CCUT", "HHN", "01", 20);
        // log10(0.5*20) = 1
        REQUIRE_THAT(station(std::pair {east2, north2}, 0, depth),
                     Catch::Matchers::WithinAbs(1 + 1.4 + 0.31, 1.e-14));
    }

    SECTION("Distance type")
    {
        REQUIRE(utahDistanceCorrections().getDistanceType() ==
                Corrections::DistanceOptions::Type::Epicentral);
        REQUIRE(yellowstoneDistanceCorrections().getDistanceType() ==
                Corrections::DistanceOptions::Type::Hypocentral);
        // Utah ignores the depth
        REQUIRE_THAT(station.getDistanceCorrection(0, depth),
                     Catch::Matchers::WithinAbs(1.4, 1.e-14));
        REQUIRE_THAT(station.getDistanceCorrection(0, -2000),
                     Catch::Matchers::WithinAbs(1.4, 1.e-14));
        // Yellowstone uses it; negative depths (above the datum) are fine
        const Magnitude::Station yellowstone{
            stationCorrection("WY", "YMR", -0.1),
            yellowstoneDistanceCorrections()};
        REQUIRE_THAT(yellowstone.getDistanceCorrection(0, 15000),
                     Catch::Matchers::WithinAbs(1.33, 1.e-14));
        REQUIRE_THAT(yellowstone.getDistanceCorrection(12000, 9000),
                     Catch::Matchers::WithinAbs(1.33, 1.e-14));
        REQUIRE_THAT(yellowstone.getDistanceCorrection(15000, -2000),
                     Catch::Matchers::WithinAbs(
                         1.33 + (std::hypot(15000.0, 2000.0) - 15000)
                               *(1.55 - 1.33)/3000, 1.e-14));
    }

    SECTION("Blank location code")
    {
        const auto east1 = amplitude("UU", "CCUT", "HHE", "", 0.3459206596016884);
        const auto north1 = amplitude("UU", "CCUT", "HHN", "  ", 0.15187045093625784);
        REQUIRE_THAT(station(std::pair {east1, north1}, distanceInMeters, depth),
                     Catch::Matchers::WithinAbs(expectedMagnitude, 1.e-10));
    }

    SECTION("Negative distance is rejected")
    {
        REQUIRE_THROWS_AS(station.getDistanceCorrection(-1, depth),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(station(std::pair {east, north}, -1, depth),
                          std::invalid_argument);
    }

    SECTION("Amplitudes need values and identifiers")
    {
        Magnitude::Amplitude noValue;
        noValue.setIdentifier(north.getIdentifier());
        REQUIRE_THROWS_AS(station(std::pair {east, noValue}, distanceInMeters, depth),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(station(std::pair {noValue, east}, distanceInMeters, depth),
                          std::invalid_argument);

        Magnitude::Amplitude noIdentifier;
        noIdentifier.setValue(0.15187045093625784);
        REQUIRE_THROWS_AS(station(std::pair {east, noIdentifier},
                                  distanceInMeters, depth),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(station(std::pair {noIdentifier, east},
                                  distanceInMeters, depth),
                          std::invalid_argument);
    }

    SECTION("Same channel is rejected")
    {
        REQUIRE_THROWS_AS(station(std::pair {east, east}, distanceInMeters, depth),
                          std::invalid_argument);
    }

    SECTION("Different stations are rejected")
    {
        const auto otherNetwork
            = amplitude("WY", "CCUT", "HHN", "01", 0.15187045093625784);
        REQUIRE_THROWS_AS(station(std::pair {east, otherNetwork},
                                  distanceInMeters, depth),
                          std::invalid_argument);
        const auto otherStation
            = amplitude("UU", "LCMT", "HHN", "01", 0.15187045093625784);
        REQUIRE_THROWS_AS(station(std::pair {east, otherStation},
                                  distanceInMeters, depth),
                          std::invalid_argument);
    }

    SECTION("Station must match the station correction")
    {
        const auto eastOther = amplitude("UU", "LCMT", "HHE", "01", 2.7);
        const auto northOther = amplitude("UU", "LCMT", "HHN", "01", 3.3);
        REQUIRE_THROWS_AS(station(std::pair {eastOther, northOther},
                                  distanceInMeters, depth),
                          std::invalid_argument);
    }

    SECTION("Different location codes are rejected")
    {
        const auto otherLocation
            = amplitude("UU", "CCUT", "HHN", "02", 0.15187045093625784);
        REQUIRE_THROWS_AS(station(std::pair {east, otherLocation},
                                  distanceInMeters, depth),
                          std::invalid_argument);
    }

    SECTION("Different instruments are rejected")
    {
        // HH vs EN
        const auto otherBand
            = amplitude("UU", "CCUT", "ENN", "01", 0.15187045093625784);
        REQUIRE_THROWS_AS(station(std::pair {east, otherBand},
                                  distanceInMeters, depth),
                          std::invalid_argument);
        // HH vs HN
        const auto otherInstrument
            = amplitude("UU", "CCUT", "HNN", "01", 0.15187045093625784);
        REQUIRE_THROWS_AS(station(std::pair {east, otherInstrument},
                                  distanceInMeters, depth),
                          std::invalid_argument);
    }

    SECTION("Copy and move")
    {
        // Copy constructor
        const Magnitude::Station copy{station};
        REQUIRE(copy.isInitialized());
        REQUIRE_THAT(copy(std::pair {east, north}, distanceInMeters, depth),
                     Catch::Matchers::WithinAbs(expectedMagnitude, 1.e-10));

        // Copy assignment
        Magnitude::Station copyAssigned{stationCorrection("UU", "LCMT", -0.12),
                                        distance};
        copyAssigned = copy;
        REQUIRE_THAT(copyAssigned.getStationCorrection(),
                     Catch::Matchers::WithinAbs(0.31, 1.e-14));
        REQUIRE_THAT(copyAssigned(std::pair {east, north}, distanceInMeters, depth),
                     Catch::Matchers::WithinAbs(expectedMagnitude, 1.e-10));

        // Move constructor
        Magnitude::Station moved{std::move(copyAssigned)};
        REQUIRE_THAT(moved(std::pair {east, north}, distanceInMeters, depth),
                     Catch::Matchers::WithinAbs(expectedMagnitude, 1.e-10));

        // Move assignment
        Magnitude::Station moveAssigned{stationCorrection("UU", "LCMT", -0.12),
                                        distance};
        moveAssigned = std::move(moved);
        REQUIRE_THAT(moveAssigned(std::pair {east, north}, distanceInMeters, depth),
                     Catch::Matchers::WithinAbs(expectedMagnitude, 1.e-10));
    }
}
