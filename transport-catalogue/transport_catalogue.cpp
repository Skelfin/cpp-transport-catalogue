#include "transport_catalogue.h"
#include <algorithm>
#include <unordered_set>
#include <vector>
#include <stdexcept>

namespace transport_catalogue {

    void TransportCatalogue::AddStop(const Stop& stop) {
        stops_[stop.name] = stop;
        stop_names_.push_back(stop.name); // Сохранение строк
    }

    void TransportCatalogue::AddBus(const Bus& bus) {
        buses_[bus.name] = bus;
        for (const auto& stop : bus.stops) {
            auto stop_it = std::find(stop_names_.begin(), stop_names_.end(), stop);
            if (stop_it != stop_names_.end()) {
                buses_by_stop_[*stop_it].insert(bus.name);
            }
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

    const Stop* TransportCatalogue::FindStop(const std::string& name) const {
        auto it = stops_.find(name);
        if (it == stops_.end()) {
            return nullptr;
        }
        return &it->second;
    }

    const Bus* TransportCatalogue::FindBus(const std::string& name) const {
        auto it = buses_.find(name);
        if (it == buses_.end()) {
            return nullptr;
        }
        return &it->second;
    }

}
