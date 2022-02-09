#include <iostream>
#include <fstream>
#include <list>
#include <random>
#include <csignal>
#include <unistd.h> // execl(), fork()
#include <stdlib.h>
#include <sys/wait.h> // waitpid()
#include <time.h>
#include <getopt.h>
#include "../include/json.hpp"

//start--------config.h---------//
// Paths of cache,config,events
#ifdef __linux__

#define HOME std::string(std::getenv("HOME")).c_str()

#define EVENT_PATH std::string(HOME).append("/.local/share/launch-meet").c_str()
#define EVENT_FILE_PATH std::string(EVENT_PATH).append("/events.json").c_str()

#define CACHE_PATH std::string(HOME).append("/.cache/launch-meet").c_str()
#define EVENT_CACHE_PATH std::string(CACHE_PATH).append("/event_cache.json").c_str()
#define EVENT_DETAILS_CACHE_PATH std::string(CACHE_PATH).append("/details_cache.json").c_str()

#define CONFIG_PATH std::string(HOME).append("/.config/launch-meet").c_str()
#define CONFIG_FILE_PATH std::string(CONFIG_PATH).append("/config.json").c_str()

#elif _WIN32

#define EVENT_FILE_PATH "events.json"
#define EVENT_CACHE_PATH "event_cache.json"
#define EVENT_DETAILS_CACHE_PATH "details_cache.json"
#define CONFIG_FILE_PATH "config.json"
#else

#endif

#define TIME_FORMAT "%Y-%m-%dT%H:%M:%S%z"
#define DAY_SEC 86400 // No. of seconds in a day
//end--------config.h---------//

// function prototype
time_t stringToEpoch(const std::string &time_str);
std::string incMonth(const std::string &cur_time, const std::string &start_time);
std::string getNow(const time_t offset = 0);
std::string epochToString(const time_t &epoch);
void incMonth(std::string &start_time);

namespace lmspace
{
    struct timings
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
    bool eventPriority(lmspace::timings first, lmspace::timings second);
    bool isExpired(nlohmann::detail::iter_impl<nlohmann::json> &itr, std::string &current_time);
    void clean();
    void init();
    void json2details(nlohmann::detail::iter_impl<nlohmann::json> &itr, struct details &event_detail);
    void json2list(const nlohmann::json &j, std::list<lmspace::timings> &event_list_cache);
    void json2map(const nlohmann::json &j, std::map<std::string, lmspace::details> &event_details_cache);
    void launchEvent(const lmspace::timings &e, const lmspace::details &d);
    void list2json(nlohmann::json &j, const std::list<lmspace::timings> &event_list_cache);
    void loadCache(std::list<lmspace::timings> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache);
    void map2json(nlohmann::json &j, const std::map<std::string, lmspace::details> &event_details_cache);
    void printDetails(const lmspace::timings &e, const lmspace::details &d);
    void updateSingleCache(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr, std::list<lmspace::timings> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache);
    void updateCache(std::list<lmspace::timings> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache);
    void updateDaysCache(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr, std::list<lmspace::timings> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache, int rep_interval);
    void updateMonthlyCache(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr, std::list<lmspace::timings> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache);
    void updateYearlyCache(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr, std::list<lmspace::timings> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache);
    void writeCache(const std::list<lmspace::timings> &event_list_cache, const std::map<std::string, lmspace::details> &event_details_cache);
    bool isexpired(const lmspace::timings &timing);
    bool is_empty(std::ifstream &pFile);
    // Functions

    // checks if file empty
    bool is_empty(std::ifstream &pFile)
    {
        return pFile.peek() == std::ifstream::traits_type::eof();
    }

    bool isexpired(const lmspace::timings &timing)
    {
        return timing.end < getNow();
    }

    void init()
    {
#ifdef __linux__
        // Creating all the required directories
        pid_t pid = fork();
        int status;
        if (!pid)
            execl("/usr/bin/mkdir", "/usr/bin/mkdir", "-m=700", "-p", EVENT_PATH, (char *)0);

        waitpid(-1, &status, 0);
        pid = fork();
        if (!pid)
            execl("/usr/bin/mkdir", "mkdir", "-m=700", "-p", CACHE_PATH, (char *)0);

        waitpid(-1, &status, 0);
        pid = fork();
        if (!pid)
            execl("/usr/bin/mkdir", "mkdir", "-m=700", "-p", CONFIG_PATH, (char *)0);
        waitpid(-1, &status, 0);
#endif

        // Initializing the required files
        std::ifstream input(EVENT_FILE_PATH);
        if (!input.is_open() || is_empty(input))
        {
            input.close();
            std::ofstream output(EVENT_FILE_PATH);
            output << "[\n]";
            output.close();
        }
        input.close();

        input.open(CONFIG_FILE_PATH);
        if (!input.is_open())
        {
            input.close();
            std::ofstream output(CONFIG_FILE_PATH);
            auto conf_json = R"(
{
    "auto-launch": true,
    "default": {
        "repeatition": false,
        "authuser": 0,
        "browser": "default",
        "description": "No description provided",
        "expiry_date_offset":31536000,
        "repeatition_interval": "1",
        "title": "Sample Title",
        "url": "https://example.com"
    },
    "interactive":false
}
)"_json;
            output << conf_json.dump(4);
            output.close();
        }
        input.close();
    }

    //  Function to clean the expired events.
    void clean()
    {
        nlohmann::json j;
        std::ifstream input(EVENT_FILE_PATH);
        if (!input.is_open())
        {
            std::cerr << "launch-meet : cannot access \'" << EVENT_FILE_PATH << "\' : " << strerror(errno) << '\n';
            exit(1);
        }
        input >> j;
        input.close();

        std::string current_time = getNow();
        for (nlohmann::json::iterator event_ptr = j.begin(); event_ptr != j.end(); ++event_ptr)
        {
            auto expiry_date = (*event_ptr)["expiry_date"].get<std::string>();
            if (expiry_date < current_time)
                j.erase(event_ptr);
        }
        std::ofstream output(EVENT_FILE_PATH);
        output << j.dump();
        output.close();

        // Success message
        std::cout << "Successfully cleaned expired events!\n";
        exit(0);
    }

    void loadCache(std::list<lmspace::timings> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache)
    {
        std::ifstream evlist_in(EVENT_CACHE_PATH);
        std::ifstream dt_in(EVENT_DETAILS_CACHE_PATH);
        if (evlist_in.is_open())
        {
            if (dt_in.is_open())
            {
                nlohmann::json ev_json, dt_json;
                evlist_in >> ev_json;
                dt_in >> dt_json;
                evlist_in.close();
                dt_in.close();
                lmspace::json2list(ev_json, event_list_cache);
                lmspace::json2map(dt_json, event_details_cache);
            }
            else
                evlist_in.close();
        }
    }

    void printDetails(const lmspace::timings &e, const lmspace::details &d)
    {
        time_t start_epoch = 0, end_epoch = 0, cur_epoch = 0;
        time(&cur_epoch);
        start_epoch = stringToEpoch(e.start);
        end_epoch = stringToEpoch(e.end);
        if (cur_epoch < start_epoch)
        {
            std::cout << "Next Event Details\n";
            std::cout << "Title      : " << d.title << '\n';
            std::cout << "Description: " << d.description << '\n';
            std::cout << "Start time : " << asctime(localtime(&start_epoch));
            std::cout << "End  time  : " << asctime(localtime(&end_epoch));
            std::cout << "Waiting " << (start_epoch - cur_epoch) / 60 << " minutes..." << std::endl;
            sleep(start_epoch - cur_epoch);
            std::cout << "Opening \"" << d.url << "\"\n\n";
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

    void launchEvent(const lmspace::timings &e, const lmspace::details &d)
    {
        lmspace::printDetails(e, d);
        std::string path, exec_name;
        if (d.browser == "default")
        {
            path = "/usr/bin/xdg-open";
            exec_name = "xdg-open";
        }
        else if (d.browser == "waterfox")
        {
            path = "/usr/local/bin/waterfox/waterfox";
            exec_name = "waterfox";
        }
        else
        {
            path = "/usr/bin/";
            path.append(d.browser);
            exec_name = d.browser;
        }
        pid_t pid = fork();
        if (!pid)
            execl(path.c_str(), exec_name.c_str(), d.url.c_str(), (char *)0);
    }

    // Function to check whether event expired or not
    bool isExpired(nlohmann::detail::iter_impl<nlohmann::json> &itr, std::string &current_time)
    {
        auto expiry_date = (*itr)["expiry_date"].get<std::string>();
        return expiry_date < current_time;
    }

    void writeCache(const std::list<lmspace::timings> &event_list_cache, const std::map<std::string, lmspace::details> &event_details_cache)
    {
        nlohmann::json cache_json, dt_json;
        lmspace::list2json(cache_json, event_list_cache);
        lmspace::map2json(dt_json, event_details_cache);

        // Writing to cache.json
        std::ofstream evlist_out(EVENT_CACHE_PATH);
        evlist_out << cache_json.dump(4);
        evlist_out.close();

        // Writing details cache to file
        std::ofstream dvlist(EVENT_DETAILS_CACHE_PATH);
        dvlist << dt_json.dump(4);
        dvlist.close();
    }

    void json2details(const nlohmann::detail::iter_impl<nlohmann::json> &itr, struct details &event_detail)
    {
        (*itr).at("authuser").get_to(event_detail.authuser);
        (*itr).at("browser").get_to(event_detail.browser);
        (*itr).at("description").get_to(event_detail.description);
        (*itr).at("title").get_to(event_detail.title);
        (*itr).at("url").get_to(event_detail.url);
    }

    void json2timings(const nlohmann::detail::iter_impl<nlohmann::json> &itr, struct lmspace::timings &event_time)
    {
        (*itr).at("start").get_to(event_time.start);
        (*itr).at("id").get_to(event_time.id);
        (*itr).at("end").get_to(event_time.end);
    }

    void json2list(const nlohmann::json &j, std::list<lmspace::timings> &event_list_cache)
    {
        for (auto itr = j.begin(); itr != j.end(); ++itr)
            event_list_cache.push_back({(*itr)["end"], (*itr)["id"], (*itr)["start"]});
    }

    void list2json(nlohmann::json &j, const std::list<lmspace::timings> &event_list_cache)
    {
        for (auto itr = event_list_cache.begin(); itr != event_list_cache.end(); ++itr)
            j.push_back({{"id", itr->id}, {"start", itr->start}, {"end", itr->end}});
    }

    void json2map(const nlohmann::json &j, std::map<std::string, lmspace::details> &event_details_cache)
    {
        for (auto itr = j.begin(); itr != j.end(); ++itr)
            event_details_cache[itr.key()] = {(*itr)["authuser"], (*itr)["browser"], (*itr)["description"], (*itr)["title"], (*itr)["url"]};
    }

    void map2json(nlohmann::json &j, const std::map<std::string, lmspace::details> &event_details_cache)
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
    void updateYearlyCache(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr, std::list<lmspace::timings> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache)
    {
        std::string expiry_date = (*event_ptr)["expiry_date"];
        std::string id = (*event_ptr)["id"];
        auto timings = (*event_ptr)["timings"];

        for (nlohmann::json::iterator timing_itr = timings.begin(); timing_itr != timings.end(); ++timing_itr)
        {
            std::string cur_time = getNow();
            std::string cache_limit_time = getNow(2678400L);
            std::string start_time = (*timing_itr)["start"];
            std::string end_time = (*timing_itr)["end"];
            time_t meet_duration = stringToEpoch(end_time) - stringToEpoch(start_time);
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
                                                       lmspace::urlUpdate(event_ptr)};
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
                        end_time = epochToString(stringToEpoch(start_time) + meet_duration);
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
                                                       lmspace::urlUpdate(event_ptr)};
                        }
                        break;
                    }
                }
            }
        }
        return;
    }

    // Updates cache for "monthly" type of repetition
    void updateMonthlyCache(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr, std::list<lmspace::timings> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache)
    {
        std::string expiry_date = (*event_ptr)["expiry_date"];
        std::string id = (*event_ptr)["id"];
        auto timings = (*event_ptr)["timings"];

        for (nlohmann::json::iterator timing_itr = timings.begin(); timing_itr != timings.end(); ++timing_itr)
        {
            std::string cur_time = getNow();
            std::string cache_limit_time = getNow(2678400L);
            std::string start_time = (*timing_itr)["start"];
            std::string end_time = (*timing_itr)["end"];
            time_t meet_duration = stringToEpoch(end_time) - stringToEpoch(start_time);
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
                                                       lmspace::urlUpdate(event_ptr)};
                        }
                    }
                    else
                    {
                        start_time = incMonth(cur_time, start_time);
                        end_time = epochToString(stringToEpoch(start_time) + meet_duration);
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
                                                       lmspace::urlUpdate(event_ptr)};
                        }
                        incMonth(start_time);
                        end_time = epochToString(stringToEpoch(start_time) + meet_duration);
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
    void updateDaysCache(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr, std::list<lmspace::timings> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache, int rep_interval)
    {
        std::string expiry_date = (*event_ptr)["expiry_date"];
        std::string id = (*event_ptr)["id"];
        auto timings = (*event_ptr)["timings"];
        // repetition interval in epoch
        const time_t epoch_offset = rep_interval * DAY_SEC;
        for (nlohmann::json::iterator timing_itr = timings.begin(); timing_itr != timings.end(); ++timing_itr)
        {

            std::string cur_time = getNow();
            std::string cache_limit_time = getNow(2678400L);
            std::string start_time = (*timing_itr)["start"];
            std::string end_time = (*timing_itr)["end"];
            // Meet duration in epoch
            time_t meet_duration = stringToEpoch(end_time) - stringToEpoch(start_time);

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
                                                       lmspace::urlUpdate(event_ptr)};
                    }
                    else
                    {
                        time_t start_epoch = 0, cur_epoch = 0;
                        start_epoch = stringToEpoch(start_time);
                        cur_epoch = stringToEpoch(cur_time);
                        int offset = (int)((cur_epoch - start_epoch) / epoch_offset);
                        start_epoch += (offset * epoch_offset);
                        if ((start_epoch + meet_duration) < cur_epoch)
                            start_epoch += epoch_offset;
                        start_time = epochToString(start_epoch);
                        end_time = epochToString(start_epoch + meet_duration);
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
                                                       lmspace::urlUpdate(event_ptr)};
                        }
                        time_t start_epoch = stringToEpoch(start_time);
                        start_time = epochToString(start_epoch + epoch_offset);
                        end_time = epochToString(start_epoch + epoch_offset + meet_duration);
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
    bool eventPriority(lmspace::timings first, lmspace::timings second)
    {
        return first.start < second.start;
    }

    // Updates cache for single time event
    void updateSingleCache(nlohmann::detail::iter_impl<nlohmann::json> &event_ptr, std::list<lmspace::timings> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache)
    {
        auto timings = (*event_ptr)["timings"];
        std::string id = (*event_ptr)["id"];
        std::string cache_limit_time = getNow(2678400L);
        for (nlohmann::json::iterator timing_itr = timings.begin(); timing_itr != timings.end(); ++timing_itr)
        {

            std::string start_time = (*timing_itr)["start"];
            std::string end_time = (*timing_itr)["end"];

            // If start time is beyond cache limit
            if (start_time > cache_limit_time)
            {
                continue;
            }

            // If start time is within cache limit
            event_list_cache.push_back({end_time, id, start_time});
            if (event_details_cache.find(id) == event_details_cache.end())
                event_details_cache[id] = {(*event_ptr)["authuser"],
                                           (*event_ptr)["browser"],
                                           (*event_ptr)["description"],
                                           (*event_ptr)["title"],
                                           lmspace::urlUpdate(event_ptr)};
        }
    }

    // Updates cache.json and details.json
    void updateCache(std::list<lmspace::timings> &event_list_cache, std::map<std::string, lmspace::details> &event_details_cache)
    {
        nlohmann::json j;
        std::ifstream input(EVENT_FILE_PATH);
        if (!input.is_open())
        {
            input.close();
            std::cerr << "launch-meet : cannot access \'" << EVENT_FILE_PATH << "\' : " << strerror(errno) << '\n';
            exit(1);
        }
        input >> j;
        input.close();

        std::string current_time = getNow();

        for (nlohmann::json::iterator event_ptr = j.begin(); event_ptr != j.end(); ++event_ptr)
        {
            // If expired
            if (lmspace::isExpired(event_ptr, current_time))
            {
                continue;
            }

            // If not expired
            bool isRepeatitive = (*event_ptr)["repeatition"];
            if (isRepeatitive)
            {
                std::string rep_interval = (*event_ptr)["repeatition_interval"];
                if (rep_interval == "yearly")
                    lmspace::updateYearlyCache(event_ptr, event_list_cache, event_details_cache);
                else if (rep_interval == "monthly")
                    lmspace::updateMonthlyCache(event_ptr, event_list_cache, event_details_cache);
                else if (rep_interval == "weekly")
                    lmspace::updateDaysCache(event_ptr, event_list_cache, event_details_cache, 7);
                else if (rep_interval == "daily")
                    lmspace::updateDaysCache(event_ptr, event_list_cache, event_details_cache, 1);
                else if (atoi(rep_interval.c_str()) < 100 && atoi(rep_interval.c_str()) > 0)
                    lmspace::updateDaysCache(event_ptr, event_list_cache, event_details_cache, atoi(rep_interval.c_str()));
                else
                {
                    std::cerr << "Repeatition interval not in correct format\n";
                    std::cerr << "So taking repatition interval as 1\n";
                    lmspace::updateDaysCache(event_ptr, event_list_cache, event_details_cache, 1);
                }
            }
            else
            {
                lmspace::updateSingleCache(event_ptr, event_list_cache, event_details_cache);
            }
        }
        event_list_cache.sort(eventPriority);
    }

}

namespace argspace
{

    struct event
    {
        bool is_interactive;
        bool repeatition;
        int authuser;
        std::list<std::pair<std::string, std::string>> timings;
        std::string browser;
        std::string created;
        std::string description;
        std::string expiry_date;
        std::string id;
        std::string repeatition_interval;
        std::string title;
        std::string updated;
        std::string url;
    };
    argspace::event eventDefault();
    std::string genRandStr(std::size_t length);
    void addTiming(const char *optarg, argspace::event &ev);
    void parseAdd(const int &argc, char **argv);
    void parseArgs(const int &argc, char **argv);
    void parseHelp(const int &argc, char **argv);
    void help(std::string _help);

    void help(std::string _help = "global")
    {
        if (_help == "global")
        {
            std::cout << R"V0G0N(usage: launch-meet <command> [commmand_options]

Options:
   -v, --version  Output version information and exit
   -b             Detaches the terminal and becomes background process
Note: To kill the background process, use 'kill `ps h -C launch-meet -o pid`'

Sub commands:
   add            Adds new event
   clean          Deletes the expired event
   edit           Edit the existing event details
   export         Export the event list
   extend         Extent the expired events
   remove         Removes the event
   search         Searches the event
   sync           Sync from google calendar, or import from calendar file
   help           Shows this help message

See 'launch-meet help <command>' to read about a specific subcommand.
Report bugs at https://github.com/ms-jagadeeshan/launch-meet/issues
)V0G0N";
        }
        else if (_help == "add")
        {
            std::cout << R"V0G0N(usage: launch-meet add [options]
Adds new event to the database.

Options:
  -a, --authuser INTEGER            Authuser value for gmeet(Default:0)
  -b, --browser BROWSER_NAME        Browser to open the event(Default:system default)
  -d, --description STRING          Description of the event
  -e, --expiry EXP_TIME             Expiry time of the event(Format: 2022-03-24T14:00)
  -i, --interactive                 Interactively takes the input
  -r, --repeat INTERVAL             Repeatition interval of the event(eg. daily,3,yearly,4,etc..)
  -t, --timings EVENT_TIME          Timings of the event (Format: 2022-10-24T11:30+100M)
  -T, --title TITLE                 Title of the event
  --url URL                         Url of the event to be opened
Note: Option '-t' can be used multiple times

Example:
$ launch-meet add -i
$ launch-meet add --url "https://meet.jit.si/" \
                  -t '2022-03-24T11:30+100M' \
                  -t '2022-04-24T11:30+60M'
$ launch-meet add -T 'Title' \
                  --url 'https://meet.jit.si/' \
                  -e '2022-10-24T14:00' \
                  -t '2022-03-24T11:30+1H' \
                  -T 'My Title'

When you don't give required details(url and timings), program will ask them interactively.
Report bugs at https://github.com/ms-jagadeeshan/launch-meet/issues
)V0G0N";
        }

        exit(0);
    }
    void parseHelp(const int &argc, char **argv)
    {
        if (argc == 2)
        {
            argspace::help();
        }
        std::string _help = std::string(argv[2], std::find(argv[2], argv[2] + 10, '\0'));
        argspace::help(_help);
    }

    void addTiming(const char *optarg, argspace::event &ev)
    {
        std::string time_arg = std::string(optarg, std::find(optarg, optarg + 25, '\0'));
        int length = time_arg.length();
        if (length < 19)
        {
            std::cerr << "Invalid time format... Give timings in below format\n-t \"2022-10-24T11:30+100M\"\n";
            exit(1);
        }
        time_t start_time = 0;
        start_time = stringToEpoch(time_arg.substr(0, 16).append(":00+0530"));

        int meet_interval = atoi(time_arg.substr(17, length - 18).c_str());
        if (time_arg[length - 1] == 'H' || time_arg[length - 1] == 'h')
        {
            meet_interval *= 3600;
        }
        else if (time_arg[length - 1] == 'M' || time_arg[length - 1] == 'm')
        {
            meet_interval *= 60;
        }
        else
        {
            std::cerr << "Invalid time format... Give timings in below format\n-t \"2022-10-24T11:30+100M\"\n";
            exit(1);
        }

        ev.timings.push_back({epochToString(start_time), epochToString(start_time + meet_interval)});
    }
    std::string genRandStr(std::size_t length)
    {
        const std::string CHARACTERS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

        std::random_device random_device;
        std::mt19937 generator(random_device());
        std::uniform_int_distribution<> distribution(0, CHARACTERS.size() - 1);

        std::string random_string;

        for (std::size_t i = 0; i < length; ++i)
        {
            random_string += CHARACTERS[distribution(generator)];
        }

        return random_string;
    }
    argspace::event eventDefault()
    {
        nlohmann::json j;
        std::ifstream input(CONFIG_FILE_PATH);
        if (!input.is_open())
        {
            std::cerr << "launch-meet : cannot access \'" << EVENT_FILE_PATH << "\' : " << strerror(errno) << '\n';
            exit(1);
        }
        input >> j;
        input.close();

        argspace::event ev = {j["interactive"].get<bool>(),
                              j["default"]["repeatition"].get<bool>(),
                              j["default"]["authuser"].get<int>(),
                              {},
                              j["default"]["browser"].get<std::string>(),
                              getNow(),
                              j["default"]["description"].get<std::string>(),
                              getNow(j["default"]["expiry_date_offset"].get<int>()),
                              genRandStr(15),
                              j["default"]["repeatition_interval"].get<std::string>(),
                              j["default"]["title"].get<std::string>(),
                              getNow(),
                              j["default"]["url"].get<std::string>()};
        return ev;
    }

    void parseArgs(const int &argc, char **argv)
    {
        if (argc > 1)
        {
            std::string subcommand = argv[1];
            argv[1] = "";
            if (subcommand == "add")
                argspace::parseAdd(argc, argv);
            else if (subcommand == "clean")
                lmspace::clean();
            else if (subcommand == "edit")
            {
                std::cout << "Coming soon\n";
                exit(0);
            }
            else if (subcommand == "export")
            {
                std::cout << "Comming soon\n";
                exit(0);
            }
            else if (subcommand == "help" || subcommand == "--help")
                argspace::parseHelp(argc, argv);
            else if (subcommand == "version" || subcommand == "--version" || subcommand == "-v")
            {
                std::cout << "launch-meet 0.5\n";
                exit(0);
            }
            else if (subcommand == "-b")
            {
                daemon(0, 0);
            }
        }
    }

    void parseAdd(const int &argc, char **argv)
    {
        static struct option long_options[] =
            {
                {"authuser", required_argument, 0, 'a'},
                {"browser", required_argument, 0, 'b'},
                {"description", required_argument, 0, 'd'},
                {"expiry", required_argument, 0, 'e'},
                {"interactive", no_argument, 0, 'i'},
                {"repeat", required_argument, 0, 'r'},
                {"timing", required_argument, 0, 't'},
                {"title", required_argument, 0, 'T'},
                {"url", required_argument, 0, 0},
                {0, 0, 0, 0}};
        int c;
        int option_index = 0;
        argspace::event ev = eventDefault();
        while (1)
        {

            /* getopt_long stores the option index here. */
            c = getopt_long(argc, argv, "a:b:d:e:r:t:T:i",
                            long_options, &option_index);

            /* Detect the end of the options. */
            if (c == -1)
                break;

            switch (c)
            {
            case 0:
            {
                if (!(strcmp(long_options[option_index].name, "authuser")))
                    ev.authuser = atoi(optarg);
                else if (!(strcmp(long_options[option_index].name, "browser")))
                    ev.browser = optarg;
                else if (!(strcmp(long_options[option_index].name, "description")))
                    ev.description = optarg;
                else if (!(strcmp(long_options[option_index].name, "expiry")))
                    ev.expiry_date = optarg;
                else if (!(strcmp(long_options[option_index].name, "interactive")))
                    ev.is_interactive = true;
                else if (!(strcmp(long_options[option_index].name, "repeat")))
                {
                    ev.repeatition = true;
                    ev.repeatition_interval = optarg;
                }
                else if (!(strcmp(long_options[option_index].name, "timing")))
                    addTiming(optarg, ev);
                else if (!(strcmp(long_options[option_index].name, "title")))
                    ev.title = optarg;
                else if (!(strcmp(long_options[option_index].name, "url")))
                    ev.url = optarg;
                else if (!(strcmp(long_options[option_index].name, "interactive")))
                    ev.is_interactive = true;
                break;
            }
            case 'a':
            {
                ev.authuser = atoi(optarg);
                break;
            }
            case 'b':
            {
                ev.browser = optarg;
                break;
            }
            case 'd':
            {
                ev.description = optarg;
                break;
            }
            case 'e':
            {
                ev.expiry_date = optarg;
                break;
            }
            case 'r':
            {
                ev.repeatition = true;
                ev.repeatition_interval = optarg;
                break;
            }
            case 't':
            {
                addTiming(optarg, ev);
                break;
            }
            case 'T':
            {
                ev.title = optarg;
                break;
            }
            case 'i':
            {
                ev.is_interactive = true;
                break;
            }
            case '?':
                /* getopt_long already printed an error message. */
                break;

            default:
                abort();
            }
        }
        argspace::event evd = eventDefault();
        if (ev.is_interactive)
        {
            // Comparing with default, to check whether arguments passed or not
            if (ev.title == evd.title)
            {
                std::cout << "Enter the Event Title: ";
                std::cin >> ev.title;
            }
            if (ev.browser == evd.browser)
            {
                int op;
                do
                {
                    std::cout << "Browser Choice\n1.Firefox\n2.Waterfox\n3.Default Browser\n";
                    std::cout << "Choose one of the browser(1-3):";
                    std::cin >> op;
                } while (op < 0 && op > 3);
                if (op == 1)
                    ev.browser = "firefox";
                else if (op == 2)
                    ev.browser = "waterfox";
            }
            if (ev.url == evd.url)
            {
                std::cout << "Enter the meet url: ";
                std::cin >> ev.url;
            }
            if (ev.authuser == evd.authuser)
            {
                bool isGoogleMeet = ev.url.find("meet.google.com/") != std::string::npos;
                if (isGoogleMeet)
                {
                    std::cout << "Enter the google meet authuser value: ";
                    std::cin >> ev.authuser;
                }
            }
            if (ev.description == evd.description)
            {
                std::cout << "Enter the Event description: ";
                std::cin >> ev.description;
            }
            if (ev.expiry_date == evd.expiry_date)
            {
                int length;
                std::string exp_date;
                do
                {
                    std::cout << "Enter expiry date(format eg. 2022-03-24T14:00): ";
                    std::cin >> exp_date;
                    length = exp_date.length();
                } while (length != 16);
                time_t exp_epoch;
                if (exp_epoch = stringToEpoch(exp_date.append(":00+0530")))
                {
                    ev.expiry_date = exp_date.append(":00+0530");
                }
                else
                {
                    std::cerr << "Invalid time format... Give timings in below format\n-t \"2022-10-24T11:30+100M\"\n";
                    exit(1);
                }
            }
            if (ev.repeatition_interval == evd.repeatition_interval)
            {
                std::cout << "Enter the repeatition interval(daily,weekly,yearly,5,3...): ";
                std::cin >> ev.repeatition_interval;
                ev.repeatition = true;
            }
            if (ev.timings.empty())
            {
                int num;
                std::string timings;
                do
                {
                    std::cout << "No. of timings of this event: ";
                    std::cin >> num;
                } while (num < 0);
                std::cout << "Give timings in below format\neg. -t \"2022-10-24T11:30+100M\"       - which means event starts at 2022-10-24 11:30AM and continues for 100 minutes\n";
                for (int i = 0; i < num; i++)
                {
                    std::cin >> timings;
                    addTiming(timings.c_str(), ev);
                }
            }
        }

        if (ev.url == evd.url)
        {
            std::cout << "Enter the meet url: ";
            std::cin >> ev.url;
        }

        if (ev.timings.empty())
        {
            int num;
            std::string timings;
            do
            {
                std::cout << "No. of timings of this event: ";
                std::cin >> num;
            } while (num < 0);
            std::cout << "\nGive timings in below format\n\tEg. -t \"2022-10-24T11:30+100M\"\t-\twhich means event starts at 2022-10-24 11:30AM and continues for 100 minutes\n";
            for (int i = 0; i < num; i++)
            {
                std::cout << "Enter the timings: ";
                std::cin >> timings;
                addTiming(timings.c_str(), ev);
            }
        }

        // Writing the changes to file
        nlohmann::json j;
        std::ifstream input(EVENT_FILE_PATH);
        if (!input.is_open())
        {
            input.close();
            std::cerr << "launch-meet : cannot access \'" << EVENT_FILE_PATH << "\' : " << strerror(errno) << '\n';
            exit(1);
        }
        input >> j;
        input.close();
        j.push_back({{"id", ev.id},
                     {"created", ev.created},
                     {"updated", ev.updated},
                     {"title", ev.title},
                     {"url", ev.url},
                     {"authuser", ev.authuser},
                     {"browser", ev.browser},
                     {"description", ev.description},
                     {"timings", ev.timings},
                     {"repeatition", ev.repeatition},
                     {"repeatition_interval", ev.repeatition_interval},
                     {"expiry_date", ev.expiry_date}});
        std::ofstream event_out(EVENT_FILE_PATH);
        event_out << j.dump();
        event_out.close();

        // Success message
        std::cout << "Successfully added the new event!\n";
        exit(0);
    }
}

void incMonth(std::string &start_time)
{
    struct tm start_tm;
    strptime(start_time.c_str(), TIME_FORMAT, &start_tm);
    start_tm.tm_zone = "IST";
    start_tm.tm_isdst = 0;
    start_tm.tm_mon += 1;
    // Buffer to hold iso-8601 seconds format eg.  2022-01-22T11:39:13+0530
    char buffer[25];
    strftime(buffer, 25, TIME_FORMAT, &start_tm);
    buffer[24] = '\0';

    start_time = std::string(buffer, std::find(buffer, buffer + 25, '\0'));
}

std::string incMonth(const std::string &cur_time, const std::string &start_time)
{
    struct tm cur_tm, start_tm;
    strptime(cur_time.c_str(), TIME_FORMAT, &cur_tm);
    strptime(start_time.c_str(), TIME_FORMAT, &start_tm);
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
    strftime(buffer, 25, TIME_FORMAT, &start_tm);
    buffer[24] = '\0';

    // coverting char* to std::string
    return std::string(buffer, std::find(buffer, buffer + 25, '\0'));
}

// Gets the current time as string, and can give offset to current time
std::string getNow(const time_t offset)
{
    // getting raw time and converting to local time
    time_t epoch;
    time(&epoch);
    epoch += offset;
    tm *tm_now = localtime(&epoch);

    // Buffer to hold iso-8601 seconds format eg.  2022-01-22T11:39:13+0530
    char buffer[25];
    strftime(buffer, 25, TIME_FORMAT, tm_now);
    buffer[24] = '\0';

    // coverting char* to std::string
    return std::string(buffer, std::find(buffer, buffer + 25, '\0'));
}

// Function converts epoch to string in iso-8601 seconds format and returns time string
std::string epochToString(const time_t &epoch)
{
    //converting to local time
    tm *tm_now = localtime(&epoch);

    // Buffer to hold iso-8601 seconds format
    //eg.  2022-01-22T11:39:13+0530
    char buffer[25];
    strftime(buffer, 25, TIME_FORMAT, tm_now);
    buffer[24] = '\0';

    // coverting char* to std::string
    return std::string(buffer, std::find(buffer, buffer + 25, '\0'));
}
void signalHandler(int signal_num)
{
    std::cout << "Bye\n";

    // It terminates the  program
    exit(signal_num);
}
// Function converts string time(in iso-8601 seconds format) to epoch and returns epoch
time_t stringToEpoch(const std::string &time_str)
{
    struct tm tm_then;
    time_t now = 0;
    strptime(time_str.c_str(), TIME_FORMAT, &tm_then);
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
    signal(SIGINT, signalHandler);
    lmspace::init();
    argspace::parseArgs(argc, argv);
    std::list<lmspace::timings> event_list_cache;
    std::map<std::string, lmspace::details> event_details_cache;

    // loading cache file if exists
    lmspace::loadCache(event_list_cache, event_details_cache);

    // If list is empty, then updates cache and writes to file
    if (event_list_cache.empty())
    {
        lmspace::updateCache(event_list_cache, event_details_cache);
        if (!event_list_cache.empty())
            lmspace::writeCache(event_list_cache, event_details_cache);
    }

    // Cleaning expired event cache and writing the remaining events
    event_list_cache.remove_if(lmspace::isexpired);
    if (!event_list_cache.empty())
        lmspace::writeCache(event_list_cache, event_details_cache);

    // Iterating events
    for (auto event = event_list_cache.begin(); event != event_list_cache.end(); ++event)
    {
        lmspace::timings current_event = (*event);
        lmspace::details current_event_details, tmp;
        auto itr = event_details_cache.find(current_event.id);
        if (itr != event_details_cache.end())
        {
            current_event_details = itr->second;
            lmspace::launchEvent(current_event, current_event_details);
        }
    }
}