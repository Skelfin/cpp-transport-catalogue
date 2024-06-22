#include "transport_catalogue.h"
#include <algorithm>
#include <unordered_set>
#include <vector>
#include <stdexcept>

namespace transport_catalogue {

    void TransportCatalogue::AddStop(const Stop& stop) {
        stops_.emplace(stop.name, stop);
        stop_string_views_.push_back(stops_.at(stop.name).name);
    }


    void TransportCatalogue::AddBus(const Bus& bus) {
        auto& added_bus = buses_.emplace(bus.name, Bus{ bus.name, {}, bus.is_circular }).first->second;

        for (const auto& stop_name : bus.stops) {
            const auto& stop_ref = stops_.at(std::string(stop_name));
            added_bus.stops.push_back(stop_ref.name);
            buses_by_stop_[stop_ref.name].insert(added_bus.name);
        }
    }


    void TransportCatalogue::SetDistance(const std::string_view& from, const std::string_view& to, int distance) {
        const Stop* from_stop = FindStop(from);
        const Stop* to_stop = FindStop(to);
        if (from_stop && to_stop) {
            distances_.emplace(std::make_pair(from_stop, to_stop), distance);
        }
    }

    const std::vector<std::string_view>& TransportCatalogue::GetBusStops(const std::string_view& bus_name) const {
        auto bus_it = buses_.find(std::string(bus_name));
        if (bus_it != buses_.end()) {
            return bus_it->second.stops;
        }
        else {
            static const std::vector<std::string_view> empty_result;
            return empty_result;
        }
    }

    BusInfo TransportCatalogue::GetBusInfo(const std::string_view& bus_name) const {
        auto bus_it = buses_.find(std::string(bus_name));
        if (bus_it == buses_.end()) {
            throw std::out_of_range("Bus not found");
        }

        const auto& bus = bus_it->second;
        std::unordered_set<std::string_view> unique_stops(bus.stops.begin(), bus.stops.end());
        double total_length = 0;
        double geo_length = 0;

        for (size_t i = 0; i + 1 < bus.stops.size(); ++i) {
            const auto& from = stops_.at(std::string(bus.stops[i]));
            const auto& to = stops_.at(std::string(bus.stops[i + 1]));
            total_length += GetDistance(bus.stops[i], bus.stops[i + 1]);
            geo_length += ComputeDistance(from.coordinates, to.coordinates);
        }

        double curvature = geo_length > 0 ? total_length / geo_length : 0;

        BusInfo bus_info;
        bus_info.name = bus.name;
        bus_info.count_stops = bus.stops.size();
        bus_info.unique_count_stops = unique_stops.size();
        bus_info.len = total_length;
        bus_info.curvature = curvature;

        return bus_info;
    }

    std::vector<std::string> TransportCatalogue::GetBusesByStop(const std::string_view& stop_name) const {
        auto stop_it = stops_.find(std::string(stop_name));
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

    const Bus* TransportCatalogue::FindBus(const std::string_view& name) const {
        auto it = buses_.find(std::string(name));
        if (it == buses_.end()) {
            return nullptr;
        }
        return &it->second;
    }

    int TransportCatalogue::GetDistance(const std::string_view& from, const std::string_view& to) const {
        const Stop* from_stop = FindStop(from);
        const Stop* to_stop = FindStop(to);

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

    size_t TransportCatalogue::PairHasher::operator()(const std::pair<const Stop*, const Stop*>& pair) const {
        return std::hash<const void*>()(pair.first) ^ std::hash<const void*>()(pair.second);
    }
}
