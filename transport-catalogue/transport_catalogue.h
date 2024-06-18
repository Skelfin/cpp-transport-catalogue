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
        std::vector<std::string> stops;
        bool is_circular;
    };

    class TransportCatalogue {
    public:
        void AddStop(const Stop& stop);
        void AddBus(const Bus& bus);
        void SetDistance(const std::string& from, const std::string& to, int distance);

        const Stop* FindStop(const std::string_view& name) const;
        const Bus* FindBus(const std::string& name) const;
        const std::vector<std::string>& GetBusStops(const std::string& bus_name) const;
        std::tuple<int, int, double, double> GetBusInfo(const std::string& bus_name) const;
        std::vector<std::string> GetBusesByStop(const std::string& stop_name) const;
        int GetDistance(const std::string& from, const std::string& to) const;

    private:
        struct PairHasher {
            size_t operator()(const std::pair<const Stop*, const Stop*>& pair) const {
                return std::hash<const void*>()(pair.first) ^ std::hash<const void*>()(pair.second);
            }
        };

        std::unordered_map<std::string, Stop> stops_;
        std::unordered_map<std::string, Bus> buses_;
        std::unordered_map<std::string_view, std::unordered_set<std::string>> buses_by_stop_;
        std::unordered_map<std::pair<const Stop*, const Stop*>, int, PairHasher> distances_;
        std::vector<std::string> stop_names_;
    };

}
