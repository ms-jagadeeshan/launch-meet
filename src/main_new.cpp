#include <iostream>
#include <fstream>
#include <list>
#include <unistd.h> // execl
#include <stdlib.h>
#include <time.h>
#include "json.hpp"

#define DAY_SEC 86400 // No. of seconds in a day
// function prototype
time_t string_to_epoch(const std::string &time_str);
std::string inc_month(const std::string &cur_time, const std::string &start_time);
std::string get_now(const time_t offset = 0);
std::string epoch_to_string(const time_t &epoch);
void inc_month(std::string &start_time);

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

    // Function prototypes

    bool event_priority(lmspace::event first, lmspace::event second);
    bool isExpired(nlohmann::detail::iter_impl<nlohmann::json> &itr, std::string &current_time);
    void clean(nlohmann::json &j);
    void json_to_details(nlohmann::detail::iter_impl<nlohmann::json> &itr, struct details &event_detail);
    void json_to_map(const nlohmann::json &j, std::map<std::string, lmspace::details> &event_details_cache);
    void json_to_list(const nlohmann::json &j, std::list<lmspace::event> &event_list_cache);
    void list_to_json(nlohmann::json &j, const std::list<lmspace::event> &event_list_cache);
    void map_to_json(nlohmann::json &j, const std::map<std::string, lmspace::details> &event_details_cache);
    void update_cache(std::list<lmspace::event> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache);
    void update_days_cache(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr, std::list<lmspace::event> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache, int rep_interval);
    void update_monthly_cache(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr, std::list<lmspace::event> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache);
    void update_yearly_cache(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr, std::list<lmspace::event> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache);
    void load_cache(std::list<lmspace::event> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache);
    void write_cache(const std::list<lmspace::event> &event_list_cache, const std::map<std::string, lmspace::details> &event_details_cache);
    void launch_event(const lmspace::event &e, const lmspace::details &d);
    void print_details(const lmspace::event &e, const lmspace::details &d);

    // Functions

    //  Function to clean the expired events.
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
    void load_cache(std::list<lmspace::event> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache)
    {
        std::ifstream evlist_in("cache.json");
        std::ifstream dt_in("details_cache.json");
        if (evlist_in.is_open())
        {
            if (dt_in.is_open())
            {
                nlohmann::json ev_json, dt_json;
                evlist_in >> ev_json;
                dt_in >> dt_json;
                lmspace::json_to_list(ev_json, event_list_cache);
                lmspace::json_to_map(dt_json, event_details_cache);
            }
            else
            {
                std::cerr << "Cache file not found!\n";
            }
        }
        else
        {
            std::cerr << "Cache file not found!\n";
        }
    }
    void print_details(const lmspace::event &e, const lmspace::details &d)
    {
        time_t start_epoch = 0, end_epoch = 0, cur_epoch = 0;
        time(&cur_epoch);
        start_epoch = string_to_epoch(e.start);
        end_epoch = string_to_epoch(e.end);
        if (cur_epoch < start_epoch)
        {
            std::cout << "Next Event Details\n";
            std::cout << "Title      : " << d.title << '\n';
            std::cout << "Description: " << d.description << '\n';
            std::cout << "Start time : " << asctime(localtime(&start_epoch));
            std::cout << "End  time  : " << asctime(localtime(&end_epoch));
            std::cout << "Waiting " << (start_epoch - cur_epoch) / 60 << " minutes...";
            sleep(start_epoch - cur_epoch);
            std::cout << "Opening " << d.url << "\n\n";
        }
        else if (cur_epoch < end_epoch)
        {
            std::cout << "Current Event Details\n";
            std::cout << "Title      : " << d.title << '\n';
            std::cout << "Description: " << d.description << '\n';
            std::cout << "Start time : " << asctime(localtime(&start_epoch));
            std::cout << "End  time  : " << asctime(localtime(&end_epoch));
            std::cout << "Opening " << d.url << "\n\n";
        }
    }

    void launch_event(const lmspace::event &e, const lmspace::details &d)
    {
        lmspace::print_details(e, d);
        std::string path,exec_name;
        if (d.browser == "default")
        {
            path = "/usr/bin/xdg-open";
            exec_name="xdg-open";
        }
        else if (d.browser == "waterfox")
        {
            path = "/usr/local/bin/waterfox/waterfox";
            exec_name="waterfox";
        }
        else
        {
            path = "/usr/bin/";
            path.append(d.browser);
            exec_name=d.browser;
        }
        execl(path.c_str(),exec_name.c_str(), d.url.c_str(), (char *)0);
    }
    // Function to check whether event expired or not
    bool isExpired(nlohmann::detail::iter_impl<nlohmann::json> &itr, std::string &current_time)
    {
        auto expiry_date = (*itr)["expiry_date"].get<std::string>();
        return expiry_date < current_time;
    }

    void write_cache(const std::list<lmspace::event> &event_list_cache, const std::map<std::string, lmspace::details> &event_details_cache)
    {
        nlohmann::json cache_json, dt_json;
        lmspace::list_to_json(cache_json, event_list_cache);
        lmspace::map_to_json(dt_json, event_details_cache);

        // Writing to cache.json
        std::ofstream evlist_out("cache.json");
        evlist_out << cache_json.dump(4);

        // Writing details cache to file
        std::ofstream dvlist("details_cache.json");
        dvlist << dt_json.dump(4).c_str();
    }

    void json_to_details(const nlohmann::detail::iter_impl<nlohmann::json> &itr, struct details &event_detail)
    {
        (*itr).at("authuser").get_to(event_detail.authuser);
        (*itr).at("browser").get_to(event_detail.browser);
        (*itr).at("description").get_to(event_detail.description);
        (*itr).at("title").get_to(event_detail.title);
        (*itr).at("url").get_to(event_detail.url);
    }

    void json_to_event(const nlohmann::detail::iter_impl<nlohmann::json> &itr, struct event &event_time)
    {
        (*itr).at("start").get_to(event_time.start);
        (*itr).at("id").get_to(event_time.id);
        (*itr).at("end").get_to(event_time.end);
    }

    void json_to_list(const nlohmann::json &j, std::list<lmspace::event> &event_list_cache)
    {
        for (auto itr = j.begin(); itr != j.end(); ++itr)
            event_list_cache.push_back({(*itr)["end"], (*itr)["id"], (*itr)["start"]});
    }

    void list_to_json(nlohmann::json &j, const std::list<lmspace::event> &event_list_cache)
    {
        for (auto itr = event_list_cache.begin(); itr != event_list_cache.end(); ++itr)
            j.push_back({{"id", itr->id}, {"start", itr->start}, {"end", itr->end}});
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
        // std::cout<<j.dump(4);
    }

    // Updates authuser value if given meet is gmeet
    std::string urlUpdate(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr)
    {
        // One liner is  same as this
        // std::string url = (*event_ptr)["url"];
        // bool isGoogleMeet = url.find("meet.google.com/") != std::string::npos;
        // if (isGoogleMeet)
        // {
        //     url.append("?authuser=").append(std::to_string((*event_ptr)["authuser"].get<int>()));
        // }
        // return url;

        return (*event_ptr)["url"].get<std::string>().find("meet.google.com/") != std::string::npos ? (*event_ptr)["url"].get<std::string>().append("?authuser=").append(std::to_string((*event_ptr)["authuser"].get<int>())) : (*event_ptr)["url"].get<std::string>();
    }

    // Updates cache for "yearly" type of repetition
    void update_yearly_cache(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr, std::list<lmspace::event> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache)
    {
        std::string expiry_date = (*event_ptr)["expiry_date"];
        std::string id = (*event_ptr)["id"];
        auto timings = (*event_ptr)["timings"];

        for (nlohmann::json::iterator timing_itr = timings.begin(); timing_itr != timings.end(); ++timing_itr)
        {
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
                        {
                            event_details_cache[id] = {(*event_ptr)["authuser"],
                                                       (*event_ptr)["browser"],
                                                       (*event_ptr)["description"],
                                                       (*event_ptr)["title"],
                                                       urlUpdate(event_ptr)};
                        }
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
                                                       urlUpdate(event_ptr)};
                        }
                        break;
                    }
                }
            }
        }
        return;
    }

    // Updates cache for "monthly" type of repetition
    void update_monthly_cache(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr, std::list<lmspace::event> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache)
    {
        std::string expiry_date = (*event_ptr)["expiry_date"];
        std::string id = (*event_ptr)["id"];
        auto timings = (*event_ptr)["timings"];

        for (nlohmann::json::iterator timing_itr = timings.begin(); timing_itr != timings.end(); ++timing_itr)
        {
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
                        {
                            event_details_cache[id] = {(*event_ptr)["authuser"],
                                                       (*event_ptr)["browser"],
                                                       (*event_ptr)["description"],
                                                       (*event_ptr)["title"],
                                                       urlUpdate(event_ptr)};
                        }
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
                                                       urlUpdate(event_ptr)};
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

    // Updates cache part for "n" type of repetition
    void update_days_cache(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr, std::list<lmspace::event> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache, int rep_interval)
    {
        std::string expiry_date = (*event_ptr)["expiry_date"];
        std::string id = (*event_ptr)["id"];
        auto timings = (*event_ptr)["timings"];
        // repetition interval in epoch
        const time_t epoch_offset = rep_interval * DAY_SEC;
        for (nlohmann::json::iterator timing_itr = timings.begin(); timing_itr != timings.end(); ++timing_itr)
        {

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
                                                       urlUpdate(event_ptr)};
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
                                                       urlUpdate(event_ptr)};
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

    // Priority determining function for events
    bool event_priority(lmspace::event first, lmspace::event second)
    {
        return first.start < second.start;
    }

    // Updates cache for single time event
    void update_single_cache(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr, std::list<lmspace::event> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache)
    {
        auto timings = (*event_ptr)["timings"];
        std::string id = (*event_ptr)["id"];
        std::string cache_limit_time = get_now(2678400L);
        for (nlohmann::json::iterator timing_itr = timings.begin(); timing_itr != timings.end(); ++timing_itr)
        {

            std::string start_time = (*timing_itr)["start"];
            std::string end_time = (*timing_itr)["end"];
            if (start_time < cache_limit_time)
            {
                event_list_cache.push_back({end_time, id, start_time});
                if (event_details_cache.find(id) == event_details_cache.end())
                    event_details_cache[id] = {(*event_ptr)["authuser"],
                                               (*event_ptr)["browser"],
                                               (*event_ptr)["description"],
                                               (*event_ptr)["title"],
                                               urlUpdate(event_ptr)};
            }
        }
    }

    // Updates cache.json and details.json
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
                    else if (atoi(rep_interval.c_str()) < 100 && atoi(rep_interval.c_str()) > 0)
                        update_days_cache(event_ptr, event_list_cache, event_details_cache, atoi(rep_interval.c_str()));
                    else
                    {
                        std::cerr << "Repeatition interval not in correct format\n";
                        std::cerr << "So taking repatition interval as 1\n";
                        update_days_cache(event_ptr, event_list_cache, event_details_cache, 1);
                    }
                }
                else
                {
                    update_single_cache(event_ptr, event_list_cache, event_details_cache);
                }
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

// Gets the current time as string, and can give offset to current time
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

    // loading cache file if exists
    lmspace::load_cache(event_list_cache, event_details_cache);

    // If list is empty, then updates cache and writes to file
    if (event_list_cache.empty())
    {
        lmspace::update_cache(event_list_cache, event_details_cache);
        if (!event_list_cache.empty())
            lmspace::write_cache(event_list_cache, event_details_cache);
    }

    // Iterating events
    for (auto event = event_list_cache.begin(); event != event_list_cache.end(); ++event)
    {
        if ((*event).end < get_now())
        {
            event_list_cache.erase(event);
            continue;
        }
        lmspace::event current_event = (*event);
        lmspace::details current_event_details, tmp;
        auto itr = event_details_cache.find(current_event.id);
        if (itr != event_details_cache.end())
        {
            current_event_details = itr->second;
            lmspace::launch_event(current_event, current_event_details);
        }
    }
}