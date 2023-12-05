#include "json-c/json.h"
#include <iostream>
#include <string>

namespace utils{
    std::string getStrFromJson(json_object *json, std::string level1, std::string level2, std::string level3);
    float getFloatFromJson(json_object *json, std::string level1, std::string level2, std::string level3);
}