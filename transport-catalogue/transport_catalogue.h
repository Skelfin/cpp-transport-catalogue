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

        const Stop* FindStop(const std::string& name) const;
        const Bus* FindBus(const std::string& name) const;
        const std::vector<std::string>& GetBusStops(const std::string& bus_name) const;
        std::tuple<int, int, double> GetBusInfo(const std::string& bus_name) const;
        std::vector<std::string> GetBusesByStop(const std::string& stop_name) const;

    private:
        std::unordered_map<std::string, Stop> stops_;
        std::unordered_map<std::string, Bus> buses_;
        std::unordered_map<std::string_view, std::unordered_set<std::string>> buses_by_stop_;
        std::vector<std::string> stop_names_;
    };

}
