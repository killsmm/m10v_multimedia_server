#include "utils.h"
#include <string.h>

std::string utils::getStrFromJson(json_object *json, std::string level1, std::string level2, std::string level3){
    json_object *tmp1;
    json_object *tmp2;
    if(!json_object_object_get_ex(json, level1.data(), &tmp1)){
        std::cout << "cannot find cmd" << std::endl;
        return "";
    }

    if(level2 == ""){
        const char *result = json_object_get_string(tmp1);
        return std::string(result, strlen(result));
    }

    if(!json_object_object_get_ex(tmp1, level2.data(), &tmp2)){
        return "";
    }

    if(level3 == ""){
        const char *result = json_object_get_string(tmp2);
        return std::string(result, strlen(result));
    }

    if(!json_object_object_get_ex(tmp2, level3.data(), &tmp1)){
        return "";
    }

    const char *result = json_object_get_string(tmp1);
    return std::string(result, strlen(result));
}

float utils::getFloatFromJson(json_object *json, std::string level1, std::string level2, std::string level3){
    json_object *tmp1;
    json_object *tmp2;
    if(!json_object_object_get_ex(json, level1.data(), &tmp1)){
        std::cout << "cannot find cmd" << std::endl;
        return 0;
    }

    if(level2 == ""){
        return json_object_get_double(tmp1);
    }

    if(!json_object_object_get_ex(tmp1, level2.data(), &tmp2)){
        return 0;
    }

    if(level3 == ""){
        return json_object_get_double(tmp2);
    }

    if(!json_object_object_get_ex(tmp2, level3.data(), &tmp1)){
        return 0;
    }

    return json_object_get_double(tmp1);
}