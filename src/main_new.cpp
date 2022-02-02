#include <iostream>
#include <fstream>
#include <list>
#include <stdlib.h>
#include <time.h>
#include "json.hpp"

#define DEBUG 1

// function prototype
std::string get_now();
time_t string_to_rawtime();

class event
{
    std::string url;
    std::string abc;
};
namespace lmspace
{
    void clean(nlohmann::json &j)
    {
        std::string current_time = get_now();
        for (nlohmann::json::iterator event_ptr = j.begin(); event_ptr != j.end(); ++event_ptr)
        {
            auto expiry_date = (*event_ptr)["expiry_date"].get<std::string>();
            if (expiry_date < current_time)
                j.erase(event_ptr);
        }
    }
    bool isExpired(nlohmann::detail::iter_impl<nlohmann::json> &itr, std::string &current_time)
    {
        auto expiry_date = (*itr)["expiry_date"].get<std::string>();
        return expiry_date < current_time;
    }

}

std::string get_now()
{
    // getting raw time and converting to local time
    time_t rawtime;
    time(&rawtime);
    tm *tm_now = localtime(&rawtime);

    // Buffer to hold iso-8601 seconds format eg.  2022-01-22T11:39:13+0530
    char buffer[25];
    strftime(buffer, 25, "%Y-%m-%dT%H:%M:%S%z", tm_now);
    buffer[24] = '\0';

    // coverting char* to std::string
    return std::string(buffer, std::find(buffer, buffer + 25, '\0'));
}

time_t string_to_rawtime()
{

    struct tm tm_then;
    time_t now = 0;
    strptime("2022-01-04T11:38:45+0530", "%Y-%m-%dT%H:%M:%S%z", &tm_then);
    tm_then.tm_zone = "IST";
    tm_then.tm_isdst = 0;
    now = mktime(&tm_then);
    if (now == -1)
    {
        std::cerr << "Error: Failed to make time\n";
        exit(1);
    }
    return now;
}

int main(int argc, char **argv)
{
    std::list<int> event_cache;
    nlohmann::json j;
    std::ifstream input("events.json");
    input >> j;
    std::string current_time = get_now();

    for (nlohmann::json::iterator event_ptr = j.begin(); event_ptr != j.end(); ++event_ptr)
    {
        if (!lmspace::isExpired(event_ptr, current_time))
        {
            bool isRepeatitive = (*event_ptr)["repeatation"].get<bool>();
            if (isRepeatitive)
            {

                auto timings = (*event_ptr)["timings"];
                for (nlohmann::json::iterator timing_itr = timings.begin(); timing_itr != timings.end(); ++timing_itr)
                {
                    if (DEBUG)
                        std::cout << (*timing_itr).dump(4);

                    auto start_time = (*timing_itr)["start"].get<std::string>();
                    auto end_time = (*timing_itr)["end"].get<std::string>();
                }

                auto expiry_date = (*event_ptr)["expiry_date"].get<std::string>();
            }

            if (DEBUG)
                std::cout << (*event_ptr).dump(4);
        }
    }
}