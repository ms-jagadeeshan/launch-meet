#include <iostream>
#include <fstream>
#include <list>
#include <stdlib.h>
#include <time.h>
#include "json.hpp"

#define DEBUG 1

#define DAY_SEC 86400 // No. of seconds in a day
// function prototype
void inc_month(std::string &start_time);
std::string inc_month(const std::string &cur_time, const std::string &start_time);
std::string get_now(const time_t offset = 0);
time_t string_to_epoch(const std::string &time_str);
std::string epoch_to_string(const time_t &epoch);

namespace lmspace
{
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
    bool isExpired(nlohmann::detail::iter_impl<nlohmann::json> &itr, std::string &current_time);
    void clean(nlohmann::json &j);
    void json_to_details(nlohmann::detail::iter_impl<nlohmann::json> &itr, struct details &event_detail);
    void json_to_map(const nlohmann::json &j, std::map<std::string, lmspace::details> &event_details_cache);
    void json_to_map(const nlohmann::json &j, std::map<std::string, lmspace::details> &event_details_cache);
    void list_to_json(nlohmann::json &j, const std::list<lmspace::event> &event_list_cache);
    void map_to_json(nlohmann::json &j, const std::map<std::string, lmspace::details> &event_details_cache);
    void update_cache(std::list<lmspace::event> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache);
    void update_days_cache(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr, std::list<lmspace::event> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache, int rep_interval);
    void update_monthly_cache(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr, std::list<lmspace::event> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache);
    void update_yearly_cache(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr, std::list<lmspace::event> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache);

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
        (*itr).at("url").get_to(event_detail.url);
        (*itr).at("browser").get_to(event_detail.browser);
        (*itr).at("description").get_to(event_detail.description);
        (*itr).at("title").get_to(event_detail.title);
        (*itr).at("authuser").get_to(event_detail.authuser);
    }
    void json_to_event(const nlohmann::detail::iter_impl<nlohmann::json> &itr, struct event &event_time)
    {
        (*itr).at("end").get_to(event_time.end);
        (*itr).at("id").get_to(event_time.id);
        (*itr).at("start").get_to(event_time.start);
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
    }
    void update_yearly_cache(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr, std::list<lmspace::event> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache)
    {
        std::string expiry_date = (*event_ptr)["expiry_date"];
        std::string id = (*event_ptr)["id"];
        auto timings = (*event_ptr)["timings"];

        for (nlohmann::json::iterator timing_itr = timings.begin(); timing_itr != timings.end(); ++timing_itr)
        {
            if (DEBUG)
                std::cout << (*timing_itr).dump(4);
            std::string cur_time = get_now();
            std::string cache_limit_time = get_now(2678400L);
            std::string start_time = (*timing_itr)["start"];
            std::string end_time = (*timing_itr)["end"];
            time_t meet_duration = string_to_epoch(end_time) - string_to_epoch(start_time);
            while (start_time < expiry_date)
            {
                if (start_time < cur_time)
                {
                    if (end_time > cur_time)
                    {
                        event_list_cache.push_back({end_time, id, start_time});
                        if (event_details_cache.find(id) == event_details_cache.end())
                            event_details_cache[id] = {(*event_ptr)["authuser"],
                                                       (*event_ptr)["browser"],
                                                       (*event_ptr)["description"],
                                                       (*event_ptr)["title"],
                                                       (*event_ptr)["url"]};
                    }
                    else
                    {
                        //If both year same, start time to next year
                        std::string cur_year = cur_time.substr(0, 3);
                        std::string start_year = start_time.substr(0, 3);
                        if (start_year == cur_year)
                        {
                            cur_year = std::to_string(atoi(start_year.c_str()) + 1);
                        }
                        start_time.replace(start_time.begin(), start_time.begin() + 3, cur_year);
                        end_time = epoch_to_string(string_to_epoch(start_time) + meet_duration);
                    }
                }
                else
                {
                    if (start_time < cache_limit_time)
                    {
                        event_list_cache.push_back({end_time, id, start_time});
                        if (event_details_cache.find(id) == event_details_cache.end())
                        {
                            event_details_cache[id] = {(*event_ptr)["authuser"],
                                                       (*event_ptr)["browser"],
                                                       (*event_ptr)["description"],
                                                       (*event_ptr)["title"],
                                                       (*event_ptr)["url"]};
                        }
                        break;
                    }
                }
            }
        }
        return;
    }
    void update_monthly_cache(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr, std::list<lmspace::event> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache)
    {
        std::string expiry_date = (*event_ptr)["expiry_date"];
        std::string id = (*event_ptr)["id"];
        auto timings = (*event_ptr)["timings"];

        for (nlohmann::json::iterator timing_itr = timings.begin(); timing_itr != timings.end(); ++timing_itr)
        {
            if (DEBUG)
                std::cout << (*timing_itr).dump(4);
            std::string cur_time = get_now();
            std::string cache_limit_time = get_now(2678400L);
            std::string start_time = (*timing_itr)["start"];
            std::string end_time = (*timing_itr)["end"];
            time_t meet_duration = string_to_epoch(end_time) - string_to_epoch(start_time);
            while (start_time < expiry_date)
            {
                if (start_time < cur_time)
                {
                    if (end_time > cur_time)
                    {
                        event_list_cache.push_back({end_time, id, start_time});
                        if (event_details_cache.find(id) == event_details_cache.end())
                            event_details_cache[id] = {(*event_ptr)["authuser"],
                                                       (*event_ptr)["browser"],
                                                       (*event_ptr)["description"],
                                                       (*event_ptr)["title"],
                                                       (*event_ptr)["url"]};
                    }
                    else
                    {
                        start_time = inc_month(cur_time, start_time);
                        end_time = epoch_to_string(string_to_epoch(start_time) + meet_duration);
                    }
                }
                else
                {
                    if (start_time < cache_limit_time)
                    {
                        event_list_cache.push_back({end_time, id, start_time});
                        if (event_details_cache.find(id) == event_details_cache.end())
                        {
                            event_details_cache[id] = {(*event_ptr)["authuser"],
                                                       (*event_ptr)["browser"],
                                                       (*event_ptr)["description"],
                                                       (*event_ptr)["title"],
                                                       (*event_ptr)["url"]};
                        }
                        inc_month(start_time);
                        end_time = epoch_to_string(string_to_epoch(start_time) + meet_duration);
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    }
    void update_days_cache(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr, std::list<lmspace::event> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache, int rep_interval)
    {
        std::string expiry_date = (*event_ptr)["expiry_date"];
        std::string id = (*event_ptr)["id"];
        auto timings = (*event_ptr)["timings"];
        // repetition interval in epoch
        const time_t epoch_offset = rep_interval * DAY_SEC;
        for (nlohmann::json::iterator timing_itr = timings.begin(); timing_itr != timings.end(); ++timing_itr)
        {
            if (DEBUG)
                std::cout << (*timing_itr).dump(4);
            std::string cur_time = get_now();
            std::string cache_limit_time = get_now(2678400L);
            std::string start_time = (*timing_itr)["start"];
            std::string end_time = (*timing_itr)["end"];
            // Meet duration in epoch
            time_t meet_duration = string_to_epoch(end_time) - string_to_epoch(start_time);

            while (start_time < expiry_date)
            {
                if (start_time < cur_time)
                {
                    if (end_time > cur_time)
                    {
                        event_list_cache.push_back({end_time, id, start_time});
                        if (event_details_cache.find(id) == event_details_cache.end())
                            event_details_cache[id] = {(*event_ptr)["authuser"],
                                                       (*event_ptr)["browser"],
                                                       (*event_ptr)["description"],
                                                       (*event_ptr)["title"],
                                                       (*event_ptr)["url"]};
                    }
                    else
                    {
                        time_t start_epoch = 0, cur_epoch = 0;
                        start_epoch = string_to_epoch(start_time);
                        cur_epoch = string_to_epoch(cur_time);
                        int offset = (int)((cur_epoch - start_epoch) / epoch_offset);
                        start_epoch += (offset * epoch_offset);
                        if ((start_epoch + meet_duration) < cur_epoch)
                            start_epoch += epoch_offset;
                        start_time = epoch_to_string(start_epoch);
                        end_time = epoch_to_string(start_epoch + meet_duration);
                    }
                }
                else
                {
                    if (start_time < cache_limit_time)
                    {
                        event_list_cache.push_back({end_time, id, start_time});
                        if (event_details_cache.find(id) == event_details_cache.end())
                        {
                            event_details_cache[id] = {(*event_ptr)["authuser"],
                                                       (*event_ptr)["browser"],
                                                       (*event_ptr)["description"],
                                                       (*event_ptr)["title"],
                                                       (*event_ptr)["url"]};
                        }
                        time_t start_epoch = string_to_epoch(start_time);
                        start_time = epoch_to_string(start_epoch + epoch_offset);
                        end_time = epoch_to_string(start_epoch + epoch_offset + meet_duration);
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    }
    bool event_priority(lmspace::event first, lmspace::event second)
    {
        return first.start < second.start;
    }
    void update_cache(std::list<lmspace::event> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache)
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
                    std::string rep_interval = (*event_ptr)["repeatition_interval"];
                    if (rep_interval == "yearly")
                        update_yearly_cache(event_ptr, event_list_cache, event_details_cache);
                    else if (rep_interval == "monthly")
                        update_monthly_cache(event_ptr, event_list_cache, event_details_cache);
                    else if (rep_interval == "weekly")
                        update_days_cache(event_ptr, event_list_cache, event_details_cache, 7);
                    else if (rep_interval == "daily")
                        update_days_cache(event_ptr, event_list_cache, event_details_cache, 1);
                    else if (atoi(rep_interval.c_str()) < 7 && atoi(rep_interval.c_str()) > 1)
                        update_days_cache(event_ptr, event_list_cache, event_details_cache, atoi(rep_interval.c_str()));
                    else
                    {
                        std::cerr << "Repeatition interval not in correct format\n";
                        std::cerr << "So taking repatition interval as 1\n";
                        update_days_cache(event_ptr, event_list_cache, event_details_cache, 1);
                    }

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
        event_list_cache.sort(event_priority);
    }
}

void inc_month(std::string &start_time)
{
    struct tm start_tm;
    strptime(start_time.c_str(), "%Y-%m-%dT%H:%M:%S%z", &start_tm);
    start_tm.tm_zone = "IST";
    start_tm.tm_isdst = 0;
    start_tm.tm_mon += 1;
    // Buffer to hold iso-8601 seconds format eg.  2022-01-22T11:39:13+0530
    char buffer[25];
    strftime(buffer, 25, "%Y-%m-%dT%H:%M:%S%z", &start_tm);
    buffer[24] = '\0';

    start_time = std::string(buffer, std::find(buffer, buffer + 25, '\0'));
}

std::string inc_month(const std::string &cur_time, const std::string &start_time)
{
    struct tm cur_tm, start_tm;
    strptime(cur_time.c_str(), "%Y-%m-%dT%H:%M:%S%z", &cur_tm);
    strptime(start_time.c_str(), "%Y-%m-%dT%H:%M:%S%z", &start_tm);
    start_tm.tm_year = cur_tm.tm_year;
    start_tm.tm_mon = cur_tm.tm_mon;
    start_tm.tm_zone = cur_tm.tm_zone = "IST";
    start_tm.tm_isdst = cur_tm.tm_isdst = 0;
    //If start time is less than current time, then increase month
    if (mktime(&start_tm) < mktime(&cur_tm))
    {
        start_tm.tm_mon += 1;
    }
    // Buffer to hold iso-8601 seconds format eg.  2022-01-22T11:39:13+0530
    char buffer[25];
    strftime(buffer, 25, "%Y-%m-%dT%H:%M:%S%z", &start_tm);
    buffer[24] = '\0';

    // coverting char* to std::string
    return std::string(buffer, std::find(buffer, buffer + 25, '\0'));
}
std::string get_now(const time_t offset)
{
    // getting raw time and converting to local time
    time_t epoch;
    time(&epoch);
    epoch += offset;
    tm *tm_now = localtime(&epoch);

    // Buffer to hold iso-8601 seconds format eg.  2022-01-22T11:39:13+0530
    char buffer[25];
    strftime(buffer, 25, "%Y-%m-%dT%H:%M:%S%z", tm_now);
    buffer[24] = '\0';

    // coverting char* to std::string
    return std::string(buffer, std::find(buffer, buffer + 25, '\0'));
}

// Function converts epoch to string in iso-8601 seconds format and returns time string
std::string epoch_to_string(const time_t &epoch)
{
    //converting to local time
    tm *tm_now = localtime(&epoch);

    // Buffer to hold iso-8601 seconds format
    //eg.  2022-01-22T11:39:13+0530
    char buffer[25];
    strftime(buffer, 25, "%Y-%m-%dT%H:%M:%S%z", tm_now);
    buffer[24] = '\0';

    // coverting char* to std::string
    return std::string(buffer, std::find(buffer, buffer + 25, '\0'));
}

// Function converts string time(in iso-8601 seconds format) to epoch and returns epoch
time_t string_to_epoch(const std::string &time_str)
{
    struct tm tm_then;
    time_t now = 0;
    strptime(time_str.c_str(), "%Y-%m-%dT%H:%M:%S%z", &tm_then);
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
        lmspace::update_cache(event_list_cache, event_details_cache);
    }

    nlohmann::json j, k;
    lmspace::map_to_json(j, event_details_cache);
    std::ofstream output("details_cache.json");
    output << j.dump(4);
    lmspace::list_to_json(k, event_list_cache);
    std::ofstream output1("cache.json");
    output1 << k.dump(4);
}