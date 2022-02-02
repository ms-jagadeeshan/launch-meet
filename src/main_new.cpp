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

namespace lmspace
{
    void clean(nlohmann::json &j);
    bool isExpired(nlohmann::detail::iter_impl<nlohmann::json> &itr, std::string &current_time);
    void json_to_details(nlohmann::detail::iter_impl<nlohmann::json> &itr, struct details &event_detail);

    struct event
    {
        std::string end;
        std::string id;
        std::string start;
    };

    struct details
    {
        int authuser;
        std::string browser;
        std::string description;
        std::string title;
        std::string url;
    };

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
    void json_to_details(const nlohmann::detail::iter_impl<nlohmann::json> &itr, struct details &event_detail)
    {
        (*itr).at("description").get_to(event_detail.description);
        (*itr).at("title").get_to(event_detail.title);
        (*itr).at("authuser").get_to(event_detail.authuser);
    }
    void json_to_list(const nlohmann::json &j, std::list<lmspace::event> &event_list_cache)
    {
        for (auto itr = j.begin(); itr != j.end(); ++itr)
            event_list_cache.push_back({(*itr)["end"], (*itr)["id"], (*itr)["start"]});
    }
    void json_to_map(const nlohmann::json &j, std::map<std::string, lmspace::details> &event_details_cache)
    {
        for (auto itr = j.begin(); itr != j.end(); ++itr)
            event_details_cache[itr.key()] = {(*itr)["authuser"], (*itr)["browser"], (*itr)["description"], (*itr)["title"], (*itr)["url"]};
    }
    void map_to_json(nlohmann::json &j, const std::map<std::string, lmspace::details> &event_details_cache)
    {
        for (std::map<std::string, lmspace::details>::const_iterator itr = event_details_cache.begin(); itr != event_details_cache.end(); ++itr)
            j[itr->first] = {{"authuser", itr->second.authuser}, {"description", itr->second.description}, {"title", itr->second.browser}, {"browser", itr->second.browser}, {"url", itr->second.url}};
    }
    void list_to_json(nlohmann::json &j, const std::list<lmspace::event> &event_list_cache)
    {
        for (auto itr = event_list_cache.begin(); itr != event_list_cache.end(); ++itr)
            j.push_back({{"id", itr->id}, {"start", itr->start}, {"end", itr->end}});
        ;
    }
    void update_cache(std::list<lmspace::event> &event_list_cache, std::map<std::string, lmspace::details> event_details_cache)
    {
        nlohmann::json j;
        std::ifstream input("events.json");
        input >> j;

        std::string current_time = get_now();

        for (nlohmann::json::iterator event_ptr = j.begin(); event_ptr != j.end(); ++event_ptr)
        {
            // if not expired
            if (!lmspace::isExpired(event_ptr, current_time))
            {
                bool isRepeatitive = (*event_ptr)["repeatition"];
                if (isRepeatitive)
                {

                    auto timings = (*event_ptr)["timings"];
                    for (nlohmann::json::iterator timing_itr = timings.begin(); timing_itr != timings.end(); ++timing_itr)
                    {
                        if (DEBUG)
                            std::cout << (*timing_itr).dump(4);

                        auto start_time = (*timing_itr)["start"];
                        auto end_time = (*timing_itr)["end"];
                    }

                    auto expiry_date = (*event_ptr)["expiry_date"];
                }

                if (DEBUG)
                    std::cout << (*event_ptr).dump(4);
            }
        }
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

    std::list<lmspace::event> event_list_cache;
    std::map<std::string, lmspace::details> event_details_cache;

    if (event_list_cache.empty())
    {
        //lmspace::update_cache(event_list_cache, event_details_cache);
    }
}