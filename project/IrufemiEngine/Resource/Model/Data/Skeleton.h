#pragma once

#include "Joint.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>


struct Skeleton {
    // RootJointのIndex
    int32_t root;
    // Joint名とIndexとの辞書
    std::map<std::string, int32_t> jointMap;
    // 所属しているジョイント
    std::vector<Joint> joints;
};