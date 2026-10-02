#include <stdexcept>
#include <utility>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "uLocalMagnitudeService/magnitude/networkMagnitude.hpp"

using namespace ULocalMagnitudeService::Magnitude;

TEST_CASE("ULocalMagnitudeService::Magnitude::NetworkMagnitude",
          "[networkMagnitude]")
{
    SECTION("Defaults")
    {
        const NetworkMagnitude magnitude;
        REQUIRE_FALSE(magnitude.hasValue());
        REQUIRE_THROWS(magnitude.getValue());
    }

    SECTION("Value")
    {
        NetworkMagnitude magnitude;
        magnitude.setValue(2.04);
        REQUIRE(magnitude.hasValue());
        REQUIRE_THAT(magnitude.getValue(),
                     Catch::Matchers::WithinAbs(2.04, 1.e-14));
        // Negative magnitudes are fine
        magnitude.setValue(-1.2);
        REQUIRE_THAT(magnitude.getValue(),
                     Catch::Matchers::WithinAbs(-1.2, 1.e-14));
        // Bounds are inclusive
        REQUIRE_NOTHROW(magnitude.setValue(-10));
        REQUIRE_NOTHROW(magnitude.setValue(10));
    }

    SECTION("Invalid value")
    {
        NetworkMagnitude magnitude;
        REQUIRE_THROWS_AS(magnitude.setValue(-10.01), std::invalid_argument);
        REQUIRE_THROWS_AS(magnitude.setValue(10.01), std::invalid_argument);
        // A failed set doesn't leave a value behind
        REQUIRE_FALSE(magnitude.hasValue());
        // or clobber an existing one
        magnitude.setValue(3.1);
        REQUIRE_THROWS_AS(magnitude.setValue(11), std::invalid_argument);
        REQUIRE_THAT(magnitude.getValue(),
                     Catch::Matchers::WithinAbs(3.1, 1.e-14));
    }

    SECTION("Copy and move")
    {
        NetworkMagnitude magnitude;
        magnitude.setValue(1.58);
        const auto check = [](const NetworkMagnitude &result)
        {
            REQUIRE(result.hasValue());
            REQUIRE_THAT(result.getValue(),
                         Catch::Matchers::WithinAbs(1.58, 1.e-14));
        };

        // Copy constructor
        const NetworkMagnitude copy{magnitude};
        check(copy);
        // Copy is deep
        magnitude.setValue(4);
        check(copy);

        // Copy assignment
        NetworkMagnitude copyAssigned;
        copyAssigned = copy;
        check(copyAssigned);

        // Move constructor
        NetworkMagnitude moved{std::move(copyAssigned)};
        check(moved);

        // Move assignment
        NetworkMagnitude moveAssigned;
        moveAssigned = std::move(moved);
        check(moveAssigned);
    }
}
