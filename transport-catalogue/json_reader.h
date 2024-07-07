#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "svg.h"
#include <vector>

namespace json_reader {

    struct RenderSettings {
        double width = 0;
        double height = 0;
        double padding = 0;
        double line_width = 0;
        double stop_radius = 0;
        svg::Point bus_label_offset = { 0, 0 };
        svg::Point stop_label_offset = { 0, 0 };
        int bus_label_font_size = 0;
        int stop_label_font_size = 0;
        svg::Color underlayer_color = "none";
        double underlayer_width = 0;
        std::vector<svg::Color> color_palette;
    };


    void ProcessRequests(std::istream& input, std::ostream& output, transport_catalogue::TransportCatalogue& tc);
    void ProcessBaseRequests(const json::Array& base_requests, transport_catalogue::TransportCatalogue& tc);
    void ProcessStatRequests(const json::Array& stat_requests, std::ostream& output, const transport_catalogue::TransportCatalogue& tc, const RenderSettings& render_settings);

}  // namespace json_reader
