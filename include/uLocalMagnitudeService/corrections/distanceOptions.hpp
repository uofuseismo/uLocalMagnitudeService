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
    void setCorrections(const std::vector<std::pair<double, double>> &distanceCorrectionPairs);
    /// @result The distance/correction table.
    /// @throws std::runtime_error if \c hasCorrections() is false.
    [[nodiscard]] std::vector<std::pair<double, double>> getCorrections() const;
    /// @result A reference to the distanc/correction table.
    /// @throws std::runtime_error if \c hasCorrections() is false.
    [[nodiscard]] const std::vector<std::pair<double, double>> &getCorrectionsReference() const;
    /// @result True indicates that the corrections were set.
    [[nodiscard]] bool hasCorrections() const noexcept;

    /// @param[in] interpolation  The interpolation mode.
    void setInterpolation(const Interpolation interpolation) noexcept;
    /// @result The interpolation mode.
    /// @note By default this is nearest so as to match what was historically
    ///       done at UUSS in AQMS and Jiggle. 
    [[nodiscard]] Interpolation getInterpolation() const noexcept;

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
///        file. 
/// @param[in] initializationFile  The initialization file to parse.
/// @param[in] section             The section of the initialization file with
///                                the distance corrections. 
/// @result The distance corrections options.
/// @throws std::invalid_argument if the initialization file does not exist
///         or any of the parameters are invalid.
[[nodiscard]] DistanceOptions fromInitializationFile(const std::filesystem::path &initializationFile,
                                                     const std::string &section = "DistanceCorrections");
}
#endif
