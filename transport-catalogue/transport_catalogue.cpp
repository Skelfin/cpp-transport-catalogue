#include "transport_catalogue.h"
#include <algorithm>
#include <unordered_set>
#include <vector>
#include <stdexcept>

namespace transport_catalogue {

    void TransportCatalogue::AddStop(const Stop& stop) {
        stops_[stop.name] = stop;
        stop_names_.push_back(stop.name);
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

    void TransportCatalogue::SetDistance(const std::string& from, const std::string& to, int distance) {
        const Stop* from_stop = FindStop(from);
        const Stop* to_stop = FindStop(to);
        if (from_stop && to_stop) {
            distances_[{from_stop, to_stop}] = distance;
        }
    }

    const std::vector<std::string>& TransportCatalogue::GetBusStops(const std::string& bus_name) const {
        auto bus_it = buses_.find(bus_name);
        if (bus_it == buses_.end()) {
            throw std::out_of_range("Bus not found");
        }
        return bus_it->second.stops;
    }

    std::tuple<int, int, double, double> TransportCatalogue::GetBusInfo(const std::string& bus_name) const {
        auto bus_it = buses_.find(bus_name);
        if (bus_it == buses_.end()) {
            throw std::out_of_range("Bus not found");
        }

        const auto& bus = bus_it->second;
        std::unordered_set<std::string> unique_stops(bus.stops.begin(), bus.stops.end());
        double total_length = 0;
        double geo_length = 0;

        for (size_t i = 0; i + 1 < bus.stops.size(); ++i) {
            const auto& from = stops_.at(bus.stops[i]);
            const auto& to = stops_.at(bus.stops[i + 1]);
            try {
                total_length += GetDistance(bus.stops[i], bus.stops[i + 1]);
            }
            catch (const std::out_of_range&) {
                // Handle the case where distance isn't found
            }
            geo_length += ComputeDistance(from.coordinates, to.coordinates);
        }

        double curvature = geo_length > 0 ? total_length / geo_length : 0;

        return { static_cast<int>(bus.stops.size()), static_cast<int>(unique_stops.size()), total_length, curvature };
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

    const Stop* TransportCatalogue::FindStop(const std::string_view& name) const {
        auto it = stops_.find(std::string(name));
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

    int TransportCatalogue::GetDistance(const std::string& from, const std::string& to) const {
        const Stop* from_stop = FindStop(from);
        const Stop* to_stop = FindStop(to);
        if (!from_stop || !to_stop) {
            throw std::out_of_range("Distance not found");
        }

        auto it = distances_.find({ from_stop, to_stop });
        if (it != distances_.end()) {
            return it->second;
        }

        it = distances_.find({ to_stop, from_stop });
        if (it != distances_.end()) {
            return it->second;
        }

        throw std::out_of_range("Distance not found");
    }

}
