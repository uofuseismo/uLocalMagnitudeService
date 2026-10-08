#include <chrono>
#include <limits>
#include <stdexcept>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitudeService/corrections/origin.hpp"

using namespace ULocalMagnitudeService::Corrections;

namespace
{
constexpr double notANumber{std::numeric_limits<double>::quiet_NaN()};
constexpr double infinity{std::numeric_limits<double>::infinity()};

/// Evid 80157466 - roughly.
Origin makeOrigin()
{
    Origin origin;
    origin.setLatitude(39.6);
    origin.setLongitude(-111.4);
    origin.setDepth(7000);
    origin.setTime(std::chrono::nanoseconds {1759000000123456789});
    return origin;
}

void checkOrigin(const Origin &origin)
{
    REQUIRE(origin.getLatitude() == 39.6);
    REQUIRE_THAT(origin.getLongitude(),
                 Catch::Matchers::WithinAbs(360 - 111.4, 1.e-12));
    REQUIRE(origin.getDepth() == 7000);
    REQUIRE(origin.getTime() == std::chrono::nanoseconds {1759000000123456789});
}
}

TEST_CASE("ULocalMagnitudeService::Corrections::Origin", "[origin]")
{
    SECTION("Defaults")
    {
        const Origin origin;
        REQUIRE_FALSE(origin.hasLatitude());
        REQUIRE_FALSE(origin.hasLongitude());
        REQUIRE_FALSE(origin.hasDepth());
        REQUIRE_FALSE(origin.hasTime());
        REQUIRE_THROWS_AS(origin.getLatitude(), std::runtime_error);
        REQUIRE_THROWS_AS(origin.getLongitude(), std::runtime_error);
        REQUIRE_THROWS_AS(origin.getDepth(), std::runtime_error);
        REQUIRE_THROWS_AS(origin.getTime(), std::runtime_error);
    }

    SECTION("Setters and getters")
    {
        const auto origin = makeOrigin();
        REQUIRE(origin.hasLatitude());
        REQUIRE(origin.hasLongitude());
        REQUIRE(origin.hasDepth());
        REQUIRE(origin.hasTime());
        checkOrigin(origin);
    }

    SECTION("Latitude")
    {
        Origin origin;
        for (const double latitude : {-90.0, -45.5, 0.0, 45.5, 90.0})
        {
            origin.setLatitude(latitude);
            REQUIRE(origin.getLatitude() == latitude);
        }
        for (const double latitude : {-90.0001, 90.0001, -180.0, 180.0,
                                      notANumber, infinity, -infinity})
        {
            INFO("Latitude: " << latitude);
            REQUIRE_THROWS_AS(origin.setLatitude(latitude),
                              std::invalid_argument);
        }
        // A rejected latitude preserves the previous one
        REQUIRE(origin.getLatitude() == 90);
    }

    SECTION("Longitude is wrapped into [0, 360)")
    {
        Origin origin;
        const std::pair<double, double> cases[]
        {
            {   0.0,   0.0},
            { 111.4, 111.4},
            {-111.4, 248.6},
            { 359.5, 359.5},
            { 360.0,   0.0},
            {-180.0, 180.0},
            { 180.0, 180.0},
            { 471.4, 111.4},
            {-471.4, 248.6},
            {-720.0,   0.0}
        };
        for (const auto &[input, expected] : cases)
        {
            INFO("Longitude: " << input);
            origin.setLongitude(input);
            REQUIRE_THAT(origin.getLongitude(),
                         Catch::Matchers::WithinAbs(expected, 1.e-10));
            REQUIRE(origin.getLongitude() >= 0);
            REQUIRE(origin.getLongitude() < 360);
        }
    }

    SECTION("Non-finite longitudes are rejected")
    {
        Origin origin;
        origin.setLongitude(-111.4);
        for (const double longitude : {notANumber, infinity, -infinity})
        {
            INFO("Longitude: " << longitude);
            REQUIRE_THROWS_AS(origin.setLongitude(longitude),
                              std::invalid_argument);
        }
        // A rejected longitude preserves the previous one
        REQUIRE_THAT(origin.getLongitude(),
                     Catch::Matchers::WithinAbs(360 + -111.4, 1.e-12));
    }

    SECTION("Depth")
    {
        Origin origin;
        // Above sea level is negative
        for (const double depth : {-8600.0, -1500.0, 0.0, 7000.0, 900000.0})
        {
            origin.setDepth(depth);
            REQUIRE(origin.getDepth() == depth);
        }
        for (const double depth : {-8600.1, 900000.1,
                                   notANumber, infinity, -infinity})
        {
            INFO("Depth: " << depth);
            REQUIRE_THROWS_AS(origin.setDepth(depth), std::invalid_argument);
        }
        REQUIRE(origin.getDepth() == 900000);
    }

    SECTION("Time")
    {
        Origin origin;
        // Before the epoch is fine
        origin.setTime(std::chrono::nanoseconds {-1000});
        REQUIRE(origin.getTime() == std::chrono::nanoseconds {-1000});
        origin.setTime(std::chrono::seconds {1759000000});
        REQUIRE(origin.getTime()
             == std::chrono::nanoseconds {1759000000000000000});
    }

    SECTION("Copy and move")
    {
        auto origin = makeOrigin();

        // Copy constructor
        const Origin copy{origin};
        checkOrigin(copy);

        // Copy is deep
        origin.setLatitude(10);
        origin.setLongitude(10);
        origin.setDepth(10);
        origin.setTime(std::chrono::nanoseconds {10});
        checkOrigin(copy);

        // Copy assignment
        Origin copyAssigned;
        copyAssigned = copy;
        checkOrigin(copyAssigned);

        // Move constructor
        Origin moved{std::move(copyAssigned)};
        checkOrigin(moved);

        // Move assignment
        Origin moveAssigned;
        moveAssigned = std::move(moved);
        checkOrigin(moveAssigned);
    }
}
