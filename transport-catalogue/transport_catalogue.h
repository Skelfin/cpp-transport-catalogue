#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include "geo.h"

namespace transport_catalogue {

    class TransportCatalogue {
    public:
        void AddStop(const std::string& name, Coordinates coordinates);
        void AddBus(const std::string& name, const std::vector<std::string>& stops, bool is_circular);

        const std::vector<std::string>& GetBusStops(const std::string& bus_name) const;
        std::tuple<int, int, double> GetBusInfo(const std::string& bus_name) const;
        std::vector<std::string> GetBusesByStop(const std::string& stop_name) const;

    private:
        struct Stop {
            std::string name;
            Coordinates coordinates;
        };

        struct Bus {
            std::string name;
            std::vector<std::string> stops;
            bool is_circular;
        };

        std::unordered_map<std::string, Stop> stops_;
        std::unordered_map<std::string, Bus> buses_;
        std::unordered_map<std::string, std::unordered_set<std::string>> buses_by_stop_;
    };

}