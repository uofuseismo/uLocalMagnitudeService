#ifndef ULOCAL_MAGNITUDE_SERVICE_TESTING_DISTANCE_TABLES_HPP
#define ULOCAL_MAGNITUDE_SERVICE_TESTING_DISTANCE_TABLES_HPP
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
// The distance correction tables used at UUSS.  These live in the
// configuration files in production; they're reproduced here so the tests
// don't depend on files that aren't checked in.
namespace ULocalMagnitudeService::Testing
{

/// Straight from Chuck Richter.  This is used with nearest neighbor
/// interpolation on the epicentral distance.
inline const std::vector<std::pair<double, double>> utahCorrections
{
    {      0.0,   1.4}, {   5000.0,   1.4}, {  10000.0,   1.5}, {  15000.0,   1.6},
    {  20000.0,   1.7}, {  25000.0,   1.9}, {  30000.0,   2.1}, {  35000.0,   2.3},
    {  40000.0,   2.4}, {  45000.0,   2.5}, {  50000.0,   2.6}, {  55000.0,   2.7},
    {  60000.0,   2.8}, {  65000.0,   2.8}, {  70000.0,   2.8}, {  80000.0,   2.9},
    {  85000.0,   2.9}, {  90000.0,   3.0}, {  95000.0,   3.0}, { 100000.0,   3.0},
    { 110000.0,   3.1}, { 120000.0,   3.1}, { 130000.0,   3.2}, { 140000.0,   3.2},
    { 150000.0,   3.3}, { 160000.0,   3.3}, { 170000.0,   3.4}, { 180000.0,   3.4},
    { 190000.0,   3.5}, { 200000.0,   3.5}, { 210000.0,   3.6}, { 220000.0,  3.65},
    { 230000.0,   3.7}, { 240000.0,   3.7}, { 250000.0,   3.8}, { 260000.0,   3.8},
    { 270000.0,   3.9}, { 280000.0,   3.9}, { 290000.0,   4.0}, { 300000.0,   4.0},
    { 310000.0,   4.1}, { 320000.0,   4.1}, { 330000.0,   4.2}, { 340000.0,   4.2},
    { 350000.0,   4.3}, { 360000.0,   4.3}, { 370000.0,   4.3}, { 380000.0,   4.4},
    { 390000.0,   4.4}, { 400000.0,   4.5}, { 410000.0,   4.5}, { 420000.0,   4.5},
    { 430000.0,   4.6}, { 440000.0,   4.6}, { 450000.0,   4.6}, { 460000.0,   4.6},
    { 470000.0,   4.7}, { 480000.0,   4.7}, { 490000.0,   4.7}, { 500000.0,   4.7},
    { 510000.0,   4.8}, { 520000.0,   4.8}, { 530000.0,   4.8}, { 540000.0,   4.8},
    { 550000.0,   4.8}, { 560000.0,   4.9}, { 570000.0,   4.9}, { 580000.0,   4.9},
    { 590000.0,   4.9}, { 600000.0,   4.9}
};

/// The Yellowstone local magnitude distance corrections.  These are from
/// Holt et al., Recalibration of the Local Magnitude Scale for Earthquakes
/// in the Yellowstone Volcanic Region (2021).  This
/// is used with linear interpolation on the hypocentral distance.
inline const std::vector<std::pair<double, double>> yellowstoneCorrections
{
    {   3000.0,  0.64}, {   6000.0,  0.72}, {   9000.0,  0.87}, {  12000.0,  1.09},
    {  15000.0,  1.33}, {  18000.0,  1.55}, {  21000.0,  1.75}, {  25000.0,  1.94},
    {  30000.0,  2.11}, {  35000.0,  2.26}, {  40000.0,   2.4}, {  45000.0,  2.55},
    {  50000.0,  2.69}, {  55000.0,  2.82}, {  60000.0,  2.94}, {  65000.0,  3.04},
    {  70000.0,  3.11}, {  75000.0,  3.15}, {  80000.0,  3.17}, {  85000.0,  3.16},
    {  90000.0,  3.15}, {  95000.0,  3.12}, { 100000.0,  3.09}, { 105000.0,  3.07},
    { 110000.0,  3.06}, { 115000.0,  3.07}, { 120000.0,   3.1}, { 125000.0,  3.15},
    { 130000.0,  3.22}, { 135000.0,  3.29}, { 140000.0,  3.37}, { 145000.0,  3.44},
    { 150000.0,   3.5}, { 155000.0,  3.56}, { 160000.0,   3.6}, { 165000.0,  3.62},
    { 170000.0,  3.65}, { 175000.0,  3.66}, { 180000.0,  3.67}
};

/// Writes an initialization file to the temporary directory and removes it
/// when it goes out of scope.
class TemporaryIniFile
{
public:
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    TemporaryIniFile(const std::string &name, const std::string &contents) :
        mPath(std::filesystem::temp_directory_path()
            / ("uLocalMagnitudeService_" + name + ".ini"))
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

/// Creates the [DistanceCorrections] section of an initialization file.
inline std::string toIniSection(
    const std::string &interpolation,
    const std::string &distanceType,
    const std::vector<std::pair<double, double>> &corrections)
{
    std::string result{"[DistanceCorrections]\n"};
    result.append("interpolation = " + interpolation + "\n");
    result.append("distanceType = " + distanceType + "\n");
    int i{1};
    for (const auto &[distance, correction] : corrections)
    {
        result.append("distance_correction_" + std::to_string(i) + " = "
                    + std::to_string(distance) + ", "
                    + std::to_string(correction) + "\n");
        ++i;
    }
    return result;
}

/// The Utah initialization file contents.
inline std::string utahIniSection()
{
    return toIniSection("nearest", "epicentral", utahCorrections);
}

/// The Yellowstone initialization file contents.
inline std::string yellowstoneIniSection()
{
    return toIniSection("linear", "hypocentral", yellowstoneCorrections);
}

}
#endif
