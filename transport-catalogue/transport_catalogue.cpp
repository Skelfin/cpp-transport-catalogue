#include "transport_catalogue.h"
#include <algorithm>
#include <unordered_set>
#include <vector>
#include <stdexcept>

namespace transport_catalogue {

    void TransportCatalogue::AddStop(const std::string& name, Coordinates coordinates) {
        stops_[name] = { name, coordinates };
    }

    void TransportCatalogue::AddBus(const std::string& name, const std::vector<std::string>& stops, bool is_circular) {
        buses_[name] = { name, stops, is_circular };
        for (const auto& stop : stops) {
            buses_by_stop_[stop].insert(name);
        }
    }

    const std::vector<std::string>& TransportCatalogue::GetBusStops(const std::string& bus_name) const {
        auto bus_it = buses_.find(bus_name);
        if (bus_it == buses_.end()) {
            throw std::out_of_range("Bus not found");
        }
        return bus_it->second.stops;
    }

    std::tuple<int, int, double> TransportCatalogue::GetBusInfo(const std::string& bus_name) const {
        auto bus_it = buses_.find(bus_name);
        if (bus_it == buses_.end()) {
            throw std::out_of_range("Bus not found");
        }

        const auto& bus = bus_it->second;
        std::unordered_set<std::string> unique_stops(bus.stops.begin(), bus.stops.end());
        double total_length = 0;

        for (size_t i = 0; i + 1 < bus.stops.size(); ++i) {
            total_length += ComputeDistance(stops_.at(bus.stops[i]).coordinates, stops_.at(bus.stops[i + 1]).coordinates);
        }

        return { static_cast<int>(bus.stops.size()), static_cast<int>(unique_stops.size()), total_length };
    }

    std::vector<std::string> TransportCatalogue::GetBusesByStop(const std::string& stop_name) const {
        auto stop_it = stops_.find(stop_name);
        if (stop_it == stops_.end()) {
            throw std::out_of_range("Stop not found");
        }

        auto buses_it = buses_by_stop_.find(stop_name);
        if (buses_it == buses_by_stop_.end()) {
            return {};
        }

        std::vector<std::string> buses(buses_it->second.begin(), buses_it->second.end());
        std::sort(buses.begin(), buses.end());
        return buses;
    }

}