#include "transport_router.h"
#include <chrono>

namespace router {

    TransportRouter::TransportRouter(const transport_catalogue::TransportCatalogue& tc, const RoutingSettings& settings)
        : tc_(tc), settings_(settings) {}

    void TransportRouter::BuildRouter() {
        const auto& stops = tc_.GetStops();
        graph_ = std::make_unique<graph::DirectedWeightedGraph<double>>(stops.size() * 2);

        graph::VertexId vertex_id = 0;
        for (const auto& [stop_name, stop] : stops) {
            stop_to_vertex_id_[stop_name] = vertex_id;
            vertex_id_to_stop_name_[vertex_id] = stop_name;
            vertex_id += 2;
        }

        // Add "Wait" edges
        for (const auto& [stop_name, _] : stops) {
            graph::EdgeId edge_id = graph_->AddEdge({ stop_to_vertex_id_[stop_name], stop_to_vertex_id_[stop_name] + 1, static_cast<double>(settings_.bus_wait_time.count()) });
            edge_id_to_info_[edge_id] = EdgeInfo{ "", 0 }; // Empty bus_name indicates a "Wait" edge
        }

        for (const auto& [bus_name, bus] : tc_.GetBuses()) {
            std::vector<std::string_view> route_stops = bus.stops;
            if (!bus.is_circular) {
                for (size_t i = bus.stops.size() - 2; i < bus.stops.size(); --i) {
                    route_stops.push_back(bus.stops[i]);
                    if (i == 0) break;
                }
            }

            for (size_t i = 0; i < route_stops.size() - 1; ++i) {
                double total_distance = 0.0;
                size_t span_count = 0;
                for (size_t j = i + 1; j < route_stops.size(); ++j) {
                    auto from_stop = route_stops[j - 1];
                    auto to_stop = route_stops[j];
                    total_distance += tc_.GetDistance(from_stop, to_stop);
                    ++span_count;

                    double travel_time = total_distance / (settings_.bus_velocity * 1000.0 / 60.0);

                    graph::EdgeId edge_id = graph_->AddEdge({ stop_to_vertex_id_[route_stops[i]] + 1, stop_to_vertex_id_[route_stops[j]], travel_time });

                    edge_id_to_info_[edge_id] = EdgeInfo{ bus_name, span_count };
                }
            }
        }

        router_ = std::make_unique<graph::Router<double>>(*graph_);
    }

    const graph::Router<double>& TransportRouter::GetRouter() const {
        return *router_;
    }

    const graph::DirectedWeightedGraph<double>& TransportRouter::GetGraph() const {
        return *graph_;
    }

    std::optional<graph::VertexId> TransportRouter::GetStopVertexId(const std::string_view& stop_name) const {
        auto it = stop_to_vertex_id_.find(stop_name);
        if (it != stop_to_vertex_id_.end()) {
            return it->second;
        }
        else {
            return std::nullopt;
        }
    }

    const EdgeInfo& TransportRouter::GetEdgeInfo(graph::EdgeId edge_id) const {
        return edge_id_to_info_.at(edge_id);
    }

    std::string_view TransportRouter::GetStopNameByVertexId(graph::VertexId vertex_id) const {
        return vertex_id_to_stop_name_.at(vertex_id);
    }

}  // namespace router