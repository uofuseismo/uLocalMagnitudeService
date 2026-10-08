#include <limits>
#include <stdexcept>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitudeService/corrections/hypocenter.hpp"

using namespace ULocalMagnitudeService::Corrections;

namespace
{
constexpr double notANumber{std::numeric_limits<double>::quiet_NaN()};
constexpr double infinity{std::numeric_limits<double>::infinity()};

/// Evid 80157466 - roughly.
Hypocenter makeHypocenter()
{
    Hypocenter hypocenter;
    hypocenter.setLatitude(39.6);
    hypocenter.setLongitude(-111.4);
    hypocenter.setDepth(7000);
    return hypocenter;
}

void checkHypocenter(const Hypocenter &hypocenter)
{
    REQUIRE(hypocenter.getLatitude() == 39.6);
    REQUIRE_THAT(hypocenter.getLongitude(),
                 Catch::Matchers::WithinAbs(360 - 111.4, 1.e-12));
    REQUIRE(hypocenter.getDepth() == 7000);
}
}

TEST_CASE("ULocalMagnitudeService::Corrections::Hypocenter", "[hypocenter]")
{
    SECTION("Defaults")
    {
        const Hypocenter hypocenter;
        REQUIRE_FALSE(hypocenter.hasLatitude());
        REQUIRE_FALSE(hypocenter.hasLongitude());
        REQUIRE_FALSE(hypocenter.hasDepth());
        REQUIRE_THROWS_AS(hypocenter.getLatitude(), std::runtime_error);
        REQUIRE_THROWS_AS(hypocenter.getLongitude(), std::runtime_error);
        REQUIRE_THROWS_AS(hypocenter.getDepth(), std::runtime_error);
    }

    SECTION("Setters and getters")
    {
        const auto hypocenter = makeHypocenter();
        REQUIRE(hypocenter.hasLatitude());
        REQUIRE(hypocenter.hasLongitude());
        REQUIRE(hypocenter.hasDepth());
        checkHypocenter(hypocenter);
    }

    SECTION("Latitude")
    {
        Hypocenter hypocenter;
        for (const double latitude : {-90.0, -45.5, 0.0, 45.5, 90.0})
        {
            hypocenter.setLatitude(latitude);
            REQUIRE(hypocenter.getLatitude() == latitude);
        }
        for (const double latitude : {-90.0001, 90.0001, -180.0, 180.0,
                                      notANumber, infinity, -infinity})
        {
            INFO("Latitude: " << latitude);
            REQUIRE_THROWS_AS(hypocenter.setLatitude(latitude),
                              std::invalid_argument);
        }
        // A rejected latitude preserves the previous one
        REQUIRE(hypocenter.getLatitude() == 90);
    }

    SECTION("Longitude is wrapped into [0, 360)")
    {
        Hypocenter hypocenter;
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
            hypocenter.setLongitude(input);
            REQUIRE_THAT(hypocenter.getLongitude(),
                         Catch::Matchers::WithinAbs(expected, 1.e-10));
            REQUIRE(hypocenter.getLongitude() >= 0);
            REQUIRE(hypocenter.getLongitude() < 360);
        }
    }

    SECTION("Non-finite longitudes are rejected")
    {
        Hypocenter hypocenter;
        hypocenter.setLongitude(-111.4);
        for (const double longitude : {notANumber, infinity, -infinity})
        {
            INFO("Longitude: " << longitude);
            REQUIRE_THROWS_AS(hypocenter.setLongitude(longitude),
                              std::invalid_argument);
        }
        // A rejected longitude preserves the previous one
        REQUIRE_THAT(hypocenter.getLongitude(),
                     Catch::Matchers::WithinAbs(360 + -111.4, 1.e-12));
    }

    SECTION("Depth")
    {
        Hypocenter hypocenter;
        // Above sea level is negative
        for (const double depth : {-8600.0, -1500.0, 0.0, 7000.0, 900000.0})
        {
            hypocenter.setDepth(depth);
            REQUIRE(hypocenter.getDepth() == depth);
        }
        for (const double depth : {-8600.1, 900000.1,
                                   notANumber, infinity, -infinity})
        {
            INFO("Depth: " << depth);
            REQUIRE_THROWS_AS(hypocenter.setDepth(depth), std::invalid_argument);
        }
        REQUIRE(hypocenter.getDepth() == 900000);
    }

    SECTION("Copy and move")
    {
        auto hypocenter = makeHypocenter();

        // Copy constructor
        const Hypocenter copy{hypocenter};
        checkHypocenter(copy);

        // Copy is deep
        hypocenter.setLatitude(10);
        hypocenter.setLongitude(10);
        hypocenter.setDepth(10);
        checkHypocenter(copy);

        // Copy assignment
        Hypocenter copyAssigned;
        copyAssigned = copy;
        checkHypocenter(copyAssigned);

        // Move constructor
        Hypocenter moved{std::move(copyAssigned)};
        checkHypocenter(moved);

        // Move assignment
        Hypocenter moveAssigned;
        moveAssigned = std::move(moved);
        checkHypocenter(moveAssigned);
    }
}
