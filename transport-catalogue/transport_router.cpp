#include "transport_router.h"

#include <chrono>

namespace router {

    TransportRouter::TransportRouter(const transport_catalogue::TransportCatalogue& tc, const RoutingSettings& settings)
        : tc_(tc), settings_(settings) {
        BuildRouter();
    }

    void TransportRouter::BuildRouter() {
        InitializeVertexIds();
        AddWaitEdges();
        AddBusEdges();
        router_ = std::make_unique<graph::Router<double>>(*graph_);
    }

    void TransportRouter::InitializeVertexIds() {
        const auto& stops = tc_.GetStops();
        graph_ = std::make_unique<graph::DirectedWeightedGraph<double>>(stops.size() * 2);

        graph::VertexId vertex_id = 0;
        for (const auto& [stop_name, stop] : stops) {
            stop_to_vertex_id_[stop_name] = vertex_id;
            vertex_id_to_stop_name_[vertex_id] = stop_name;
            vertex_id += 2;
        }
    }

    void TransportRouter::AddWaitEdges() {
        for (const auto& [stop_name, _] : tc_.GetStops()) {
            graph::EdgeId edge_id = graph_->AddEdge({
                stop_to_vertex_id_[stop_name],
                stop_to_vertex_id_[stop_name] + 1,
                static_cast<double>(settings_.bus_wait_time.count())
                });
            edge_id_to_info_[edge_id] = EdgeInfo{ "", 0 };
        }
    }

    void TransportRouter::AddBusEdges() {
        for (const auto& [bus_name, bus] : tc_.GetBuses()) {
            const auto& stops = bus.stops;

            if (stops.size() < 2) {
                continue;
            }

            auto AddEdgesBetweenStops = [&](const std::vector<std::string_view>& stop_sequence) {
                for (size_t i = 0; i + 1 < stop_sequence.size(); ++i) {
                    double total_distance = 0.0;
                    size_t span_count = 0;

                    for (size_t j = i + 1; j < stop_sequence.size(); ++j) {
                        const auto from_stop = stop_sequence[j - 1];
                        const auto to_stop = stop_sequence[j];

                        total_distance += tc_.GetDistance(from_stop, to_stop);
                        ++span_count;

                        double bus_velocity_m_per_min = settings_.bus_velocity * 1000.0 / 60.0;
                        double travel_time = total_distance / bus_velocity_m_per_min;

                        graph::EdgeId edge_id = graph_->AddEdge({
                            stop_to_vertex_id_[stop_sequence[i]] + 1,
                            stop_to_vertex_id_[stop_sequence[j]],
                            travel_time
                            });

                        edge_id_to_info_[edge_id] = EdgeInfo{ bus_name, span_count };
                    }
                }
                };

            AddEdgesBetweenStops(stops);

            if (!bus.is_circular) {
                std::vector<std::string_view> reverse_stops(stops.rbegin(), stops.rend());
                AddEdgesBetweenStops(reverse_stops);
            }
        }
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
        vertex_id = vertex_id % 2 == 0 ? vertex_id : vertex_id - 1;
        auto it = vertex_id_to_stop_name_.find(vertex_id);
        if (it != vertex_id_to_stop_name_.end()) {
            return it->second;
        }
        else {
            return "";
        }
    }

    std::optional<RouteInfo> TransportRouter::FindOptimalRoute(const std::string& from, const std::string& to) const {
        auto from_vertex_opt = GetStopVertexId(from);
        auto to_vertex_opt = GetStopVertexId(to);

        if (!from_vertex_opt || !to_vertex_opt) {
            return std::nullopt;
        }

        graph::VertexId start_vertex = from_vertex_opt.value();
        graph::VertexId end_vertex = to_vertex_opt.value();

        auto route_result = router_->BuildRoute(start_vertex, end_vertex);

        if (!route_result) {
            return std::nullopt;
        }

        RouteInfo route_info;
        route_info.total_time = route_result->weight;

        for (const auto& edge_id : route_result->edges) {
            const auto& edge = graph_->GetEdge(edge_id);
            const auto& edge_info = GetEdgeInfo(edge_id);

            if (edge_info.bus_name.empty()) {
                std::string_view stop_name = GetStopNameByVertexId(edge.from);
                route_info.items.push_back(RouteItem{
                    RouteItem::Type::Wait,
                    std::string(stop_name),
                    "",
                    edge.weight,
                    0
                    });
            }
            else {
                route_info.items.push_back(RouteItem{
                    RouteItem::Type::Bus,
                    "",
                    edge_info.bus_name,
                    edge.weight,
                    edge_info.span_count
                    });
            }
        }

        return route_info;
    }

}  // namespace router
