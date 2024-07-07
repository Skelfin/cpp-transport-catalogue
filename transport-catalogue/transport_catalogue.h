#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <string_view>
#include <optional>
#include "domain.h"

namespace transport_catalogue {

    class TransportCatalogue {
    public:
        void AddStop(const domain::Stop& stop);
        void AddBus(const domain::Bus& bus);
        void SetDistance(const std::string_view& from, const std::string_view& to, int distance);

        const domain::Stop* FindStop(const std::string_view& name) const;
        const domain::Bus* FindBus(const std::string_view& name) const;
        const std::vector<std::string_view>& GetBusStops(const std::string_view& bus_name) const;
        domain::BusInfo GetBusInfo(const std::string_view& bus_name) const;
        std::optional<std::vector<std::string>> GetBusesByStop(const std::string_view& stop_name) const;
        int GetDistance(const std::string_view& from, const std::string_view& to) const;

        const std::unordered_map<std::string, domain::Bus>& GetBuses() const;
        const std::unordered_map<std::string, domain::Stop>& GetStops() const;
        void UpdateFilteredStops(const std::unordered_set<std::string>& stops_in_routes);


    private:
        struct PairHasher {
            size_t operator()(const std::pair<const domain::Stop*, const domain::Stop*>& pair) const;
        };

        std::unordered_map<std::string, domain::Stop> stops_;
        std::unordered_map<std::string, domain::Bus> buses_;
        std::unordered_map<std::string_view, std::unordered_set<std::string>> buses_by_stop_;
        std::unordered_map<std::pair<const domain::Stop*, const domain::Stop*>, int, PairHasher> distances_;
        std::vector<std::string_view> stop_string_views_;
        std::unordered_map<std::string, domain::Stop> filtered_stops_;
    };

}  // namespace transport_catalogue
