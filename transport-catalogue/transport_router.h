#pragma once

#include "graph.h"
#include "transport_catalogue.h"
#include <memory>
#include <chrono>
#include "router.h"
#include <optional>

namespace router {

    struct RoutingSettings {
        std::chrono::minutes bus_wait_time;
        double bus_velocity;
    };

    struct EdgeInfo {
        std::string bus_name;
        size_t span_count;
    };

    class TransportRouter {
    public:
        TransportRouter(const transport_catalogue::TransportCatalogue& tc, const RoutingSettings& settings);

        void BuildRouter();

        const graph::Router<double>& GetRouter() const;
        const graph::DirectedWeightedGraph<double>& GetGraph() const;
        std::optional<graph::VertexId> GetStopVertexId(const std::string_view& stop_name) const;
        const EdgeInfo& GetEdgeInfo(graph::EdgeId edge_id) const;
        std::string_view GetStopNameByVertexId(graph::VertexId vertex_id) const;

    private:
        const transport_catalogue::TransportCatalogue& tc_;
        RoutingSettings settings_;
        std::unique_ptr<graph::DirectedWeightedGraph<double>> graph_;
        std::unique_ptr<graph::Router<double>> router_;
        std::unordered_map<std::string_view, graph::VertexId> stop_to_vertex_id_;
        std::unordered_map<graph::VertexId, std::string_view> vertex_id_to_stop_name_;
        std::unordered_map<graph::EdgeId, EdgeInfo> edge_id_to_info_;
    };

}  // namespace router