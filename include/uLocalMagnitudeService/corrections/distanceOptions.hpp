#ifndef ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_DISTANCE_OPTIONS_HPP
#define ULOCAL_MAGNITUDE_SERVICE_CORRECTIONS_DISTANCE_OPTIONS_HPP
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>
namespace ULocalMagnitudeService::Corrections
{   
 class DistanceOptions;
}
namespace ULocalMagnitudeService::Corrections
{

/// @class DistanceOptions distanceOptions.hpp
/// @brief Defines the distance correction options.
/// @copyright Ben Baker (University of Utah) distributed under the
///            MIT NO AI license.
class DistanceOptions
{
public:
    enum class Interpolation
    {
        Nearest,  /*!< Interpolates the correction to the nearest abscissa. */
        Linear    /*!< Linearly interpolates between nodes and transitions
                       to a constant at the extrema. */
    };
    enum class Type
    {
        Epicentral, /*!< Distance corrections are based on epicentral distance. */
        Hypocentral /*!< Distance corrections are based on hypocentral distance. */
    };
public:
    /// @brief Constructor.
    DistanceOptions();
    /// @brief Copy constructor.
    DistanceOptions(const DistanceOptions &options);
    /// @brief Move constructor.
    DistanceOptions(DistanceOptions &&options) noexcept;
 
    /// @brief Sets the table of distance/corrections. 
    /// @param[in] distanceCorrectionsPairs  For each element, pair.first is the
    ///                                      (source-receiver) distance in meters
    ///                                      and pair.second is the corresponding
    ///                                      correction in magnitude units.
    /// @throws std::invalid_argument if distanceCorrectionsPairs is empty,
    ///         contains a duplicate distance, has a distance less than 0.
    /// @note The meaning of distance in this table is defined in \c getType().
    void setCorrections(const std::vector<std::pair<double, double>> &distanceCorrectionPairs);
    /// @result The distance/correction table.
    /// @throws std::runtime_error if \c hasCorrections() is false.
    [[nodiscard]] std::vector<std::pair<double, double>> getCorrections() const;
    /// @result A reference to the distanc/correction table.
    /// @throws std::runtime_error if \c hasCorrections() is false.
    [[nodiscard]] const std::vector<std::pair<double, double>> &getCorrectionsReference() const;
    /// @result True indicates that the corrections were set.
    [[nodiscard]] bool hasCorrections() const noexcept;

    /// @brief Sets the interpolation type.  
    /// @note For what it's worth, Utah uses nearest neighbor while Yellowstone
    ///       uses linear.
    /// @param[in] interpolation  The interpolation mode.
    void setInterpolation(const Interpolation interpolation) noexcept;
    /// @result The interpolation mode.
    /// @throws std::runtime_error if \c hasInterpolation() is false.
    [[nodiscard]] Interpolation getInterpolation() const;
    /// @result True indicates the interpolation type was set.
    [[nodiscard]] bool hasInterpolation() const noexcept;

    /// @brief Sets the distance type. 
    /// @param[in] type  The distance type.
    /// @note For what's worth, Utah uses epicentral while Yellowstone
    ///       uses hypocentral.
    void setType(const Type type) noexcept;
    /// @result The distance type.  
    /// @throws std::runtime_error if \c hasType() is false.
    [[nodiscard]] Type getType() const;
    /// @result True indicates the distance type was set.
    [[nodiscard]] bool hasType() const noexcept;

    /* 
    /// @brief Sets a maximum distance after which point the model is invalid.
    /// @param[in] maximumDistance   The maximum model distance.
    void setMaximumDistance(double maximumDistance);
    /// @result The maximum model distance.
    [[nodiscard]] double getMaximumDistance() const noexcept;
    */

    /// @brief Destructor.
    ~DistanceOptions();
    /// @brief Copy assignment.
    DistanceOptions& operator=(const DistanceOptions &options);
    /// @brief Move assignment.
    DistanceOptions& operator=(DistanceOptions &&options) noexcept;
private:
    class DistanceOptionsImpl;
    std::unique_ptr<DistanceOptionsImpl> pImpl;
};

/// @brief Creates the distance corrections options from an initialization
///        file.  The section must look like:
///        @code
///        [DistanceCorrections]
///        ; nearest or linear
///        interpolation = nearest
///        ; epicentral or hypocentral
///        distanceType = epicentral
///        ; distance in meters, correction in magnitude units - numbered
///        ; from 1 with no gaps
///        distance_correction_1 = 0, 1.4
///        distance_correction_2 = 5000, 1.4
///        @endcode
/// @param[in] initializationFile  The initialization file to parse.
/// @param[in] section             The section of the initialization file with
///                                the distance corrections. 
/// @result The distance corrections options.
/// @throws std::invalid_argument if the initialization file does not exist,
///         the interpolation or distance type is missing or invalid, or
///         the table is missing or invalid.
[[nodiscard]] DistanceOptions fromInitializationFile(const std::filesystem::path &initializationFile,
                                                     const std::string &section = "DistanceCorrections");
}
#endif
