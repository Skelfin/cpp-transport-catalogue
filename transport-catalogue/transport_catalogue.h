#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <string_view>
#include "geo.h"

namespace transport_catalogue {

    struct Stop {
        std::string name;
        Coordinates coordinates;
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

    class TransportCatalogue {
    public:
        void AddStop(const Stop& stop);
        void AddBus(const Bus& bus);
        void SetDistance(const std::string_view& from, const std::string_view& to, int distance);

        const Stop* FindStop(const std::string_view& name) const;
        const Bus* FindBus(const std::string_view& name) const;
        const std::vector<std::string_view>& GetBusStops(const std::string_view& bus_name) const;
        BusInfo GetBusInfo(const std::string_view& bus_name) const;
        std::vector<std::string> GetBusesByStop(const std::string_view& stop_name) const;
        int GetDistance(const std::string_view& from, const std::string_view& to) const;

    private:
        struct PairHasher {
            size_t operator()(const std::pair<const Stop*, const Stop*>& pair) const;
        };

        std::unordered_map<std::string, Stop> stops_;
        std::unordered_map<std::string, Bus> buses_;
        std::unordered_map<std::string_view, std::unordered_set<std::string>> buses_by_stop_;
        std::unordered_map<std::pair<const Stop*, const Stop*>, int, PairHasher> distances_;
        std::vector<std::string_view> stop_string_views_;
    };

}
