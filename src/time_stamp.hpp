#include <ctime>
#include <iomanip>
#include <sstream>

std::string git_timestamp()
{
    std::time_t now = std::time(nullptr);
    std::tm *lt = std::localtime(&now);

    // Get timezone offset
    std::time_t utc_now = std::mktime(std::gmtime(&now));
    long offset = std::difftime(now, utc_now);
    int hours = offset / 3600;
    int minutes = (std::abs(offset) % 3600) / 60;

    std::ostringstream oss;
    oss << now << " "
        << (hours >= 0 ? "+" : "-")
        << std::setw(2) << std::setfill('0') << std::abs(hours)
        << std::setw(2) << std::setfill('0') << minutes;

    return oss.str();
}
