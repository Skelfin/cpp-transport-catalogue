#include "json_reader.h"
#include "request_handler.h"
#include "map_renderer.h"
#include <iostream>
#include <sstream>

namespace json_reader {

    RenderSettings ParseRenderSettings(const json::Dict& dict) {
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

    void ProcessRequests(std::istream& input, std::ostream& output, transport_catalogue::TransportCatalogue& tc) {
        const auto document = json::Load(input);
        const auto& root = document.GetRoot().AsMap();

        const auto& base_requests = root.at("base_requests").AsArray();
        const auto& stat_requests = root.at("stat_requests").AsArray();
        const auto& render_settings_dict = root.at("render_settings").AsMap();
        auto render_settings = ParseRenderSettings(render_settings_dict);

        ProcessBaseRequests(base_requests, tc);
        ProcessStatRequests(stat_requests, output, tc, render_settings);
    }

    void ProcessBaseRequests(const json::Array& base_requests, transport_catalogue::TransportCatalogue& tc) {
        std::unordered_set<std::string> stops_in_routes;


        for (const auto& request : base_requests) {
            const auto& request_map = request.AsMap();
            const std::string& type = request_map.at("type").AsString();
            if (type == "Stop") {
                const std::string& name = request_map.at("name").AsString();
                const double latitude = request_map.at("latitude").AsDouble();
                const double longitude = request_map.at("longitude").AsDouble();

                domain::Stop stop{ name, {latitude, longitude} };
                tc.AddStop(stop);
            }
        }

        for (const auto& request : base_requests) {
            const auto& request_map = request.AsMap();
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
                tc.AddBus(bus);
            }
        }

        for (const auto& request : base_requests) {
            const auto& request_map = request.AsMap();
            const std::string& type = request_map.at("type").AsString();
            if (type == "Stop") {
                const std::string& name = request_map.at("name").AsString();
                const auto& road_distances = request_map.at("road_distances").AsMap();
                for (const auto& [neighbor_name, distance_node] : road_distances) {
                    tc.SetDistance(name, neighbor_name, distance_node.AsInt());
                }
            }
        }
        tc.UpdateFilteredStops(stops_in_routes);
    }

    void ProcessStatRequests(const json::Array& stat_requests, std::ostream& output, const transport_catalogue::TransportCatalogue& tc, const RenderSettings& render_settings) {
        json::Array responses;
        request_handler::RequestHandler handler(tc);

        for (const auto& request : stat_requests) {
            const auto& request_map = request.AsMap();
            const int request_id = request_map.at("id").AsInt();
            const std::string& type = request_map.at("type").AsString();

            json::Dict response;
            response["request_id"] = request_id;

            if (type == "Stop") {
                const std::string& name = request_map.at("name").AsString();
                auto buses_opt = handler.GetBusesByStop(name);
                if (!buses_opt) {
                    response["error_message"] = "not found";
                }
                else {
                    const auto& buses = *buses_opt;
                    json::Array buses_node;
                    for (const auto& bus : buses) {
                        buses_node.emplace_back(bus);
                    }
                    response["buses"] = std::move(buses_node);
                }
            }
            else if (type == "Bus") {
                const std::string& name = request_map.at("name").AsString();
                try {
                    domain::BusInfo bus_info = handler.GetBusInfo(name);
                    response["curvature"] = bus_info.curvature;
                    response["route_length"] = static_cast<int>(bus_info.len);
                    response["stop_count"] = static_cast<int>(bus_info.count_stops);
                    response["unique_stop_count"] = static_cast<int>(bus_info.unique_count_stops);
                }
                catch (const std::out_of_range&) {
                    response["error_message"] = "not found";
                }
            }
            else if (type == "Map") {
                std::ostringstream map_stream;
                map_renderer::RenderMap(tc, map_stream, render_settings);
                const std::string map_svg = map_stream.str();

                response["map"] = map_svg;
            }

            responses.emplace_back(std::move(response));
        }

        json::Print(json::Document{ std::move(responses) }, output);
    }

}  // namespace json_reader
