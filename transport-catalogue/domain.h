#pragma once

#include <string>
#include <vector>
#include <string_view>
#include "geo.h"

namespace domain {

    struct Stop {
        std::string name;
        geo::Coordinates coordinates;
    };

    struct Bus {
        std::string name;
        std::vector<std::string_view> stops;
        bool is_circular;
    };

    struct BusInfo {
        std::string name;
        size_t count_stops;
        size_t unique_count_stops;
        double len;
        double curvature;
        BusInfo();
    };

}  // namespace domain
