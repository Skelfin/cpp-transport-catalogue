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
        BusInfo()
            : name("")
            , count_stops(0)
            , unique_count_stops(0)
            , len(0.0)
            , curvature(0.0)
        {}
        std::string name;
        size_t count_stops;
        size_t unique_count_stops;
        double len;
        double curvature;
    };

}  // namespace domain
