#include <limits>
#include <stdexcept>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include "uLocalMagnitudeService/magnitude/stationMagnitude.hpp"

using namespace ULocalMagnitudeService::Magnitude;

namespace
{
constexpr double nan{std::numeric_limits<double>::quiet_NaN()};
constexpr double infinity{std::numeric_limits<double>::infinity()};

StationMagnitude makeMagnitude()
{
    StationMagnitude magnitude;
    magnitude.setStationName("UU.CCUT");
    magnitude.setValue(2.204987145462916);
    magnitude.setStationCorrection(0.31);
    magnitude.setDistanceCorrection(2.8);
    return magnitude;
}

void checkMagnitude(const StationMagnitude &magnitude)
{
    REQUIRE(magnitude.getStationName() == "UU.CCUT");
    REQUIRE(magnitude.getValue() == 2.204987145462916);
    REQUIRE(magnitude.getStationCorrection() == 0.31);
    REQUIRE(magnitude.getDistanceCorrection() == 2.8);
}
}

TEST_CASE("ULocalMagnitudeService::Magnitude::StationMagnitude",
          "[stationMagnitude]")
{
    SECTION("Defaults")
    {
        const StationMagnitude magnitude;
        REQUIRE_FALSE(magnitude.hasValue());
        REQUIRE_FALSE(magnitude.hasStationCorrection());
        REQUIRE_FALSE(magnitude.hasDistanceCorrection());
        REQUIRE_FALSE(magnitude.hasStationName());
        REQUIRE_THROWS_AS(magnitude.getValue(), std::runtime_error);
        REQUIRE_THROWS_AS(magnitude.getStationCorrection(),
                          std::runtime_error);
        REQUIRE_THROWS_AS(magnitude.getDistanceCorrection(),
                          std::runtime_error);
        REQUIRE_THROWS_AS(magnitude.getStationName(), std::runtime_error);
    }

    SECTION("Setters and getters")
    {
        const auto magnitude = makeMagnitude();
        REQUIRE(magnitude.hasValue());
        REQUIRE(magnitude.hasStationCorrection());
        REQUIRE(magnitude.hasDistanceCorrection());
        REQUIRE(magnitude.hasStationName());
        checkMagnitude(magnitude);
    }

    SECTION("Negative and zero values are fine")
    {
        // Small events have negative magnitudes and corrections can be
        // negative or zero
        StationMagnitude magnitude;
        magnitude.setValue(-0.5);
        REQUIRE(magnitude.getValue() == -0.5);
        magnitude.setValue(0);
        REQUIRE(magnitude.getValue() == 0);
        magnitude.setStationCorrection(-0.32);
        REQUIRE(magnitude.getStationCorrection() == -0.32);
        magnitude.setStationCorrection(0);
        REQUIRE(magnitude.getStationCorrection() == 0);
        magnitude.setDistanceCorrection(0);
        REQUIRE(magnitude.getDistanceCorrection() == 0);
    }

    SECTION("Non-finite values are rejected")
    {
        auto magnitude = makeMagnitude();
        for (const auto bad : {nan, infinity, -infinity})
        {
            REQUIRE_THROWS_AS(magnitude.setValue(bad), std::invalid_argument);
            REQUIRE_THROWS_AS(magnitude.setStationCorrection(bad),
                              std::invalid_argument);
            REQUIRE_THROWS_AS(magnitude.setDistanceCorrection(bad),
                              std::invalid_argument);
        }
        // The previous values are kept
        checkMagnitude(magnitude);
    }

    SECTION("Empty station name is rejected")
    {
        StationMagnitude magnitude;
        REQUIRE_THROWS_AS(magnitude.setStationName(""), std::invalid_argument);
        REQUIRE_FALSE(magnitude.hasStationName());
        magnitude.setStationName("UU.CCUT");
        REQUIRE_THROWS_AS(magnitude.setStationName(""), std::invalid_argument);
        REQUIRE(magnitude.getStationName() == "UU.CCUT");
    }

    SECTION("Setting again overwrites")
    {
        auto magnitude = makeMagnitude();
        magnitude.setStationName("UU.LCMT");
        magnitude.setValue(2.158753817909035);
        magnitude.setStationCorrection(-0.12);
        magnitude.setDistanceCorrection(2.1);
        REQUIRE(magnitude.getStationName() == "UU.LCMT");
        REQUIRE(magnitude.getValue() == 2.158753817909035);
        REQUIRE(magnitude.getStationCorrection() == -0.12);
        REQUIRE(magnitude.getDistanceCorrection() == 2.1);
    }

    SECTION("Copy and move")
    {
        auto magnitude = makeMagnitude();

        // Copy constructor
        const StationMagnitude copy{magnitude};
        checkMagnitude(copy);

        // Copy is deep: modifying the original doesn't touch the copy
        magnitude.setStationName("UU.LCMT");
        magnitude.setValue(1);
        magnitude.setStationCorrection(1);
        magnitude.setDistanceCorrection(1);
        checkMagnitude(copy);

        // Copy assignment
        StationMagnitude copyAssigned;
        copyAssigned = copy;
        checkMagnitude(copyAssigned);

        // Move constructor
        StationMagnitude moved{std::move(copyAssigned)};
        checkMagnitude(moved);

        // Move assignment
        StationMagnitude moveAssigned;
        moveAssigned = std::move(moved);
        checkMagnitude(moveAssigned);
    }
}
