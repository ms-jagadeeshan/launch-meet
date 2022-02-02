#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <getopt.h>
#include <list>
#include <time.h>
#include "json.hpp"
using nlohmann::json;
using std::cin;
using std::cout;

class event
{
public:
    std::string expiry_date;
    bool repeatation;
};
namespace ns
{
    // void to_json(json &j, const event &p)
    // {
    //     j = json{{"name", p.name}, {"address", p.address}, {"age", p.age}};
    // }

    void from_json(const json &j, event &p)
    {
        j.at("expiry_date").get_to(p.expiry_date);
        j.at("address").get_to(p.repeatation);
    }
}

std::string get_now()
{
    // getting raw time using time()
    time_t rawtime = 0;
    time(&rawtime);

    //getting local time struct using localtime()
    tm *tm_now = localtime(&rawtime);

    // Buffer to hold iso-8601 seconds format
    // eg. 2022-01-22T11:39:13+0530
    char buffer[25];
    strftime(buffer, 30, "%Y-%m-%dT%H:%M:%S%z", tm_now);
    buffer[24] = '\0';

    // converting char* to std::string
    return std::string(buffer, std::find(buffer, buffer + 26, '\0'));
}

namespace lm
{
    void clean(json &j)
    {
        // getting current time and iterating the events
        std::string cur_time = get_now();

        for (json::iterator it = j.begin(); it != j.end(); ++it)
        {
            auto expiry_date = (*it)["expiry_date"].get<std::string>();

            cout << expiry_date << "\ndf ad";
            if (expiry_date < cur_time)
            {
                // erasing the events which are expired
                j.erase(it);
            }
        }

        // dumping the changes to classes.json
        std::ofstream output("classes.json");
        output << j.dump(4);
    }
    void help(const int opts)
    {
        switch (opts)
        {
        case 0:
            cout << "\
        df";
        }
    }
    void add();
    void extend();
    void remove();
    void search();
    void export_to();
    void sync();
    void edut();
}

// class event

// namespace ns
// {
//     void to_json(json &j, const event &p)
//     {
//         j = json{{"name", p.name}, {"address", p.address}, {"age", p.age}};
//     }

//     void from_json(const json &j, event &p)
//     {
//         j.at("name").get_to(p.name);
//         j.at("address").get_to(p.address);
//         j.at("age").get_to(p.age);
//     }
// } // namespace ns
enum options
{
    add_ = 1,
    clean_ = 2,
    default_ = 0,
    edit_ = 3,
    export_ = 4,
    extend_ = 5,
    remove_ = 6,
    search_ = 7,
    sync_ = 8
};

void parse_args(const int &argc, char **argv)
{
    if (argc < 2)
    {
        lm::help(default_);
    }
    else
    {
        if (argv[1] == "add")
            lm::add();
        else if (argv[1] == "clean")
            lm::clean();
        else if (argv[1] == "edit")
            lm::edit();
        else if (argv[1] == "export")
            lm::export_to();
        else if (argv[1] == "add")
            lm::add();
        else if (argv[1] == "add")
            lm::add();
        else if (argv[1] == "add")
            lm::add();
        else if (argv[1] == "add")
            lm::add();
        else if (argv[1] == "add")
            lm::add();
    }
}

int main(int argc, char **argv)
{

    parse_args(argc, argv);
    static int verbose_flag;

    int c;

    while (1)
    {
        static struct option long_options[] =
            {
                /* These options set a flag. */
                {"verbose", no_argument, &verbose_flag, 1},
                {"brief", no_argument, &verbose_flag, 0},
                /* These options don’t set a flag.
             We distinguish them by their indices. */
                {"add", no_argument, 0, 'a'},
                {"append", no_argument, 0, 'b'},
                {"delete", required_argument, 0, 'd'},
                {"create", required_argument, 0, 'c'},
                {"file", no_argument, 0, 'f'},
                {0, 0, 0, 0}};
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long(argc, argv, "abc:d:f:",
                        long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
        case 0:
            /* If this option set a flag, do nothing else now. */
            if (long_options[option_index].flag != 0)
                break;
            printf("option %s", long_options[option_index].name);
            if (optarg)
                printf(" with arg %s", optarg);
            printf("\n");
            break;

        case 'a':
            puts("option -a\n");
            break;

        case 'b':
            puts("option -b\n");
            break;

        case 'c':
            printf("option -c with value `%s'\n", optarg);
            break;

        case 'd':
            printf("option -d with value `%s'\n", optarg);
            break;

        case 'f':
            printf("option -f with value `%s'\n", optarg);
            break;

        case '?':
            /* getopt_long already printed an error message. */
            break;

        default:
            abort();
        }
    }

    /* Instead of reporting ‘--verbose’
     and ‘--brief’ as they are encountered,
     we report the final status resulting from them. */
    if (verbose_flag)
        puts("verbose flag is set");

    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
        printf("non-option ARGV-elements: ");
        while (optind < argc)
            printf("%s ", argv[optind++]);
        putchar('\n');
    }

    json j;
    std::ifstream input("classes.json");
    input >> j;
    std::string cur_time = get_now();
    for (json::iterator itr = j.begin(); itr != j.end(); ++itr)
    {

        // cout << (*itr).dump(4) << '\n';
        // cout << (*itr)["authuser"];
        cout << *itr << "\n";

        auto expiry_date = (*itr)["expiry_date"].get<std::string>();

        cout << expiry_date << "\ndf ad";
        if (expiry_date < cur_time)
        {
        }
        // for (json::iterator 2022-01-22T11:02:48+05:30it1 = (*it).begin(); it1 != (*it).end(); ++it)
        // {
        //     cout << "db";
        //     cout << *it1 << '\n';
        // }
    }
    cout << j.dump(4);
    //auto tmp = j["url"].get<std::string>();
}