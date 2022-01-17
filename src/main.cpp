#include <iostream>
#include <fstream>
#include <list>
#include "json.hpp"
using nlohmann::json;
using std::cin;
using std::cout;

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

int main()
{
    json j;
    j["pi"] = "1.3";
    cout << j.dump(4);
    j.erase("pi");
    std::ifstream input("classes.json");
    input >> j;
    cout << j.dump(4);
    for (json::iterator it = j.begin(); it != j.end(); ++it)
    {
        cout << *it << '\n';
        // for (json::iterator it1 = (*it).begin(); it1 != (*it).end(); ++it)
        // {
        //     cout << *it1 << '\n';
        // }
    }
    //auto tmp = j["url"].get<std::string>();
}