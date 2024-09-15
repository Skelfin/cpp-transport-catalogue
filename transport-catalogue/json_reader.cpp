#include "json_reader.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "json_builder.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace json_reader {

    RenderSettings JsonReader::ParseRenderSettings(const json::Dict& dict) {
        RenderSettings settings;
        settings.width = dict.at("width").AsDouble();
        settings.height = dict.at("height").AsDouble();
        settings.padding = dict.at("padding").AsDouble();
        settings.line_width = dict.at("line_width").AsDouble();
        settings.stop_radius = dict.at("stop_radius").AsDouble();
        settings.bus_label_font_size = dict.at("bus_label_font_size").AsInt();
        settings.bus_label_offset = { dict.at("bus_label_offset").AsArray().at(0).AsDouble(),
                                     dict.at("bus_label_offset").AsArray().at(1).AsDouble() };
        settings.stop_label_font_size = dict.at("stop_label_font_size").AsInt();
        settings.stop_label_offset = { dict.at("stop_label_offset").AsArray().at(0).AsDouble(),
                                      dict.at("stop_label_offset").AsArray().at(1).AsDouble() };

        const auto& underlayer_color = dict.at("underlayer_color");
        if (underlayer_color.IsArray()) {
            const auto& color_array = underlayer_color.AsArray();
            if (color_array.size() == 3) {
                settings.underlayer_color = svg::Rgb{
                    static_cast<uint8_t>(color_array[0].AsInt()),
                    static_cast<uint8_t>(color_array[1].AsInt()),
                    static_cast<uint8_t>(color_array[2].AsInt())
                };
            }
            else if (color_array.size() == 4) {
                settings.underlayer_color = svg::Rgba{
                    static_cast<uint8_t>(color_array[0].AsInt()),
                    static_cast<uint8_t>(color_array[1].AsInt()),
                    static_cast<uint8_t>(color_array[2].AsInt()),
                    color_array[3].AsDouble()
                };
            }
        }
        else {
            settings.underlayer_color = underlayer_color.AsString();
        }
        settings.underlayer_width = dict.at("underlayer_width").AsDouble();

        const auto& color_palette = dict.at("color_palette").AsArray();
        for (const auto& color_node : color_palette) {
            if (color_node.IsString()) {
                settings.color_palette.emplace_back(color_node.AsString());
            }
            else if (color_node.IsArray()) {
                const auto& color_array = color_node.AsArray();
                if (color_array.size() == 3) {
                    settings.color_palette.emplace_back(svg::Rgb{
                        static_cast<uint8_t>(color_array[0].AsInt()),
                        static_cast<uint8_t>(color_array[1].AsInt()),
                        static_cast<uint8_t>(color_array[2].AsInt())
                        });
                }
                else if (color_array.size() == 4) {
                    settings.color_palette.emplace_back(svg::Rgba{
                        static_cast<uint8_t>(color_array[0].AsInt()),
                        static_cast<uint8_t>(color_array[1].AsInt()),
                        static_cast<uint8_t>(color_array[2].AsInt()),
                        color_array[3].AsDouble()
                        });
                }
            }
        }

        return settings;
    }

    router::RoutingSettings JsonReader::ParseRoutingSettings(const json::Dict& dict) {
        router::RoutingSettings settings;
        settings.bus_wait_time = std::chrono::minutes(dict.at("bus_wait_time").AsInt());
        settings.bus_velocity = dict.at("bus_velocity").AsDouble();

        return settings;
    }

    json::Node JsonReader::ProcessRequests(const json::Node& input) {
        const auto& root = input.AsDict();

        ProcessBaseRequests(root.at("base_requests").AsArray());
        render_settings_ = ParseRenderSettings(root.at("render_settings").AsDict());
        routing_settings_ = ParseRoutingSettings(root.at("routing_settings").AsDict());

        // Check if router is already built
        if (!transport_router_) {
            transport_router_ = std::make_unique<router::TransportRouter>(tc_, routing_settings_);
            transport_router_->BuildRouter();
        }

        return json::Node{ ProcessStatRequests(root.at("stat_requests").AsArray()) };
    }

    void JsonReader::ProcessBaseRequests(const json::Array& base_requests) {
        std::unordered_set<std::string> stops_in_routes;

        for (const auto& request : base_requests) {
            const auto& request_map = request.AsDict();
            const std::string& type = request_map.at("type").AsString();
            if (type == "Stop") {
                const std::string& name = request_map.at("name").AsString();
                const double latitude = request_map.at("latitude").AsDouble();
                const double longitude = request_map.at("longitude").AsDouble();

                domain::Stop stop{ name, {latitude, longitude} };
                tc_.AddStop(stop);
            }
        }

        for (const auto& request : base_requests) {
            const auto& request_map = request.AsDict();
            const std::string& type = request_map.at("type").AsString();
            if (type == "Bus") {
                const std::string& name = request_map.at("name").AsString();
                const auto& stops_node = request_map.at("stops").AsArray();
                std::vector<std::string_view> stops;
                for (const auto& stop_node : stops_node) {
                    stops.emplace_back(stop_node.AsString());
                    stops_in_routes.insert(stop_node.AsString());
                }
                const bool is_roundtrip = request_map.at("is_roundtrip").AsBool();

                domain::Bus bus{ name, stops, is_roundtrip };
                tc_.AddBus(bus);
            }
        }

        for (const auto& request : base_requests) {
            const auto& request_map = request.AsDict();
            const std::string& type = request_map.at("type").AsString();
            if (type == "Stop") {
                const std::string& name = request_map.at("name").AsString();
                const auto& road_distances = request_map.at("road_distances").AsDict();
                for (const auto& [neighbor_name, distance_node] : road_distances) {
                    tc_.SetDistance(name, neighbor_name, distance_node.AsInt());
                }
            }
        }
        tc_.UpdateFilteredStops(stops_in_routes);
    }

    json::Array JsonReader::ProcessStatRequests(const json::Array& stat_requests) {
        json::Array responses;
        request_handler::RequestHandler handler(tc_);

        for (const auto& request : stat_requests) {
            const auto& request_map = request.AsDict();
            const int request_id = request_map.at("id").AsInt();
            const std::string& type = request_map.at("type").AsString();

            json::Builder response_builder;
            response_builder.StartDict().Key("request_id").Value(request_id);

            if (type == "Stop") {
                const std::string& name = request_map.at("name").AsString();
                auto buses_opt = handler.GetBusesByStop(name);
                if (!buses_opt) {
                    response_builder.Key("error_message").Value("not found");
                }
                else {
                    const auto& buses = *buses_opt;
                    json::Array buses_node;
                    for (const auto& bus : buses) {
                        buses_node.push_back(bus.name);
                    }
                    response_builder.Key("buses").Value(buses_node);
                }
            }
            else if (type == "Bus") {
                const std::string& name = request_map.at("name").AsString();
                try {
                    domain::BusInfo bus_info = handler.GetBusInfo(name);
                    response_builder.Key("curvature").Value(bus_info.curvature)
                        .Key("route_length").Value(static_cast<int>(bus_info.len))
                        .Key("stop_count").Value(static_cast<int>(bus_info.count_stops))
                        .Key("unique_stop_count").Value(static_cast<int>(bus_info.unique_count_stops));
                }
                catch (const std::out_of_range&) {
                    response_builder.Key("error_message").Value("not found");
                }
            }
            else if (type == "Map") {
                std::ostringstream map_stream;
                map_renderer::RenderMap(tc_, map_stream, render_settings_);
                const std::string map_svg = map_stream.str();
                response_builder.Key("map").Value(map_svg);
            }
            else if (type == "Route") {
                const std::string& from = request_map.at("from").AsString();
                const std::string& to = request_map.at("to").AsString();

                const auto from_stop = tc_.FindStop(from);
                const auto to_stop = tc_.FindStop(to);

                if (!from_stop || !to_stop) {
                    response_builder.Key("error_message").Value(std::string("not found"));
                }
                else {
                    auto from_id = transport_router_->GetStopVertexId(from);
                    auto to_id = transport_router_->GetStopVertexId(to);

                    if (!from_id || !to_id) {
                        response_builder.Key("error_message").Value(std::string("not found"));
                    }
                    else {
                        auto route_info = transport_router_->GetRouter().BuildRoute(*from_id, *to_id);
                        if (route_info) {
                            response_builder.Key("total_time").Value(route_info->weight);
                            json::Array items;
                            for (const auto edge_id : route_info->edges) {
                                const auto& edge_info = transport_router_->GetEdgeInfo(edge_id);
                                const auto& edge = transport_router_->GetGraph().GetEdge(edge_id);

                                if (edge_info.bus_name.empty()) {
                                    // This is a "Wait" edge
                                    std::string stop_name = std::string(transport_router_->GetStopNameByVertexId(edge.from));
                                    items.push_back(
                                        json::Builder{}
                                        .StartDict()
                                        .Key("type").Value(std::string("Wait"))
                                        .Key("stop_name").Value(stop_name)
                                        .Key("time").Value(edge.weight)
                                        .EndDict()
                                        .Build()
                                    );
                                }
                                else {
                                    // This is a "Bus" edge
                                    items.push_back(
                                        json::Builder{}
                                        .StartDict()
                                        .Key("type").Value(std::string("Bus"))
                                        .Key("bus").Value(edge_info.bus_name)
                                        .Key("span_count").Value(static_cast<int>(edge_info.span_count))
                                        .Key("time").Value(edge.weight)
                                        .EndDict()
                                        .Build()
                                    );
                                }
                            }
                            response_builder.Key("items").Value(items);
                        }
                        else {
                            response_builder.Key("error_message").Value(std::string("not found"));
                        }
                    }
                }
            }

            responses.push_back(response_builder.EndDict().Build());
        }

        return responses;
    }

}  // namespace json_reader
