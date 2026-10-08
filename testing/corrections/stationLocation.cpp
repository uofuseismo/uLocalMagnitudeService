#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitudeService/corrections/stationLocation.hpp"

using namespace ULocalMagnitudeService::Corrections;

namespace
{
constexpr double notANumber{std::numeric_limits<double>::quiet_NaN()};
constexpr double infinity{std::numeric_limits<double>::infinity()};

/// Roughly UU.CTU in Salt Lake City.
StationLocation makeLocation()
{
    StationLocation location;
    location.setLatitude(40.76);
    location.setLongitude(-111.85);
    location.setElevation(1460);
    return location;
}

void checkLocation(const StationLocation &location)
{
    REQUIRE(location.getLatitude() == 40.76);
    REQUIRE_THAT(location.getLongitude(),
                 Catch::Matchers::WithinAbs(360 - 111.85, 1.e-12));
    REQUIRE(location.getElevation() == 1460);
}
}

TEST_CASE("ULocalMagnitudeService::Corrections::StationLocation",
          "[stationLocation]")
{
    SECTION("Defaults")
    {
        const StationLocation location;
        REQUIRE_FALSE(location.hasLatitude());
        REQUIRE_FALSE(location.hasLongitude());
        REQUIRE_FALSE(location.hasElevation());
        REQUIRE_THROWS_AS(location.getLatitude(), std::runtime_error);
        REQUIRE_THROWS_AS(location.getLongitude(), std::runtime_error);
        REQUIRE_THROWS_AS(location.getElevation(), std::runtime_error);
    }

    SECTION("Setters and getters")
    {
        const auto location = makeLocation();
        REQUIRE(location.hasLatitude());
        REQUIRE(location.hasLongitude());
        REQUIRE(location.hasElevation());
        checkLocation(location);
    }

    SECTION("Latitude")
    {
        StationLocation location;
        for (const double latitude : {-90.0, -45.5, 0.0, 45.5, 90.0})
        {
            location.setLatitude(latitude);
            REQUIRE(location.getLatitude() == latitude);
        }
        for (const double latitude : {-90.0001, 90.0001, -180.0, 180.0,
                                      notANumber, infinity, -infinity})
        {
            INFO("Latitude: " << latitude);
            REQUIRE_THROWS_AS(location.setLatitude(latitude),
                              std::invalid_argument);
        }
        // A rejected latitude preserves the previous one
        REQUIRE(location.getLatitude() == 90);
    }

    SECTION("Longitude is wrapped into [0, 360)")
    {
        StationLocation location;
        const std::pair<double, double> cases[]
        {
            {    0.0,    0.0},
            { 111.85, 111.85},
            {-111.85, 248.15},
            {  360.0,    0.0},
            { -180.0,  180.0},
            {  471.85, 111.85},
            { -720.0,    0.0}
        };
        for (const auto &[input, expected] : cases)
        {
            INFO("Longitude: " << input);
            location.setLongitude(input);
            REQUIRE_THAT(location.getLongitude(),
                         Catch::Matchers::WithinAbs(expected, 1.e-10));
            REQUIRE(location.getLongitude() >= 0);
            REQUIRE(location.getLongitude() < 360);
        }
    }

    SECTION("Non-finite longitudes are rejected")
    {
        // setLongitude is noexcept so a bad value can't throw - it must not
        // be accepted either
        StationLocation location;
        location.setLongitude(-111.85);
        for (const double longitude : {notANumber, infinity, -infinity})
        {
            INFO("Longitude: " << longitude);
            location.setLongitude(longitude);
            REQUIRE(std::isfinite(location.getLongitude()));
        }
    }

    SECTION("Elevation")
    {
        StationLocation location;
        // Boreholes and ocean-bottom stations are below sea level
        for (const double elevation : {-10000.0, -150.0, 0.0, 1460.0,
                                       4421.0, 8600.0})
        {
            INFO("Elevation: " << elevation);
            location.setElevation(elevation);
            REQUIRE(location.getElevation() == elevation);
        }
        for (const double elevation : {-10000.1, 8600.1,
                                       notANumber, infinity, -infinity})
        {
            INFO("Elevation: " << elevation);
            REQUIRE_THROWS_AS(location.setElevation(elevation),
                              std::invalid_argument);
        }
    }

    SECTION("Copy and move")
    {
        auto location = makeLocation();

        // Copy constructor
        const StationLocation copy{location};
        checkLocation(copy);

        // Copy is deep
        location.setLatitude(10);
        location.setLongitude(10);
        location.setElevation(10);
        checkLocation(copy);

        // Copy assignment
        StationLocation copyAssigned;
        copyAssigned = copy;
        checkLocation(copyAssigned);

        // Move constructor
        StationLocation moved{std::move(copyAssigned)};
        checkLocation(moved);

        // Move assignment
        StationLocation moveAssigned;
        moveAssigned = std::move(moved);
        checkLocation(moveAssigned);
    }
}
