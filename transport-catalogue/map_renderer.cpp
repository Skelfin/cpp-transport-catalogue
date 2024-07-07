#include "map_renderer.h"
#include <algorithm>

namespace map_renderer {

    void RenderMap(const transport_catalogue::TransportCatalogue& tc, std::ostream& output, const json_reader::RenderSettings& settings) {
        std::vector<geo::Coordinates> coordinates;
        for (const auto& [name, stop] : tc.GetStops()) {
            if (!tc.GetBusesByStop(name).has_value()) continue;
            coordinates.emplace_back(stop.coordinates);
        }

        SphereProjector projector(coordinates.begin(), coordinates.end(), settings.width, settings.height, settings.padding);

        svg::Document doc;

        std::vector<std::string> bus_names;
        for (const auto& [name, bus] : tc.GetBuses()) {
            bus_names.emplace_back(name);
        }
        std::sort(bus_names.begin(), bus_names.end());

        size_t color_index = 0;

        for (const auto& name : bus_names) {
            const auto& bus = tc.GetBuses().at(name);
            if (bus.stops.empty()) continue;

            svg::Polyline polyline;
            const auto& color = settings.color_palette[color_index % settings.color_palette.size()];
            polyline.SetStrokeColor(color)
                .SetFillColor(svg::NoneColor)
                .SetStrokeWidth(settings.line_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            for (const auto& stop_name : bus.stops) {
                const auto& stop = tc.GetStops().at(std::string(stop_name));
                auto point = projector(stop.coordinates);
                polyline.AddPoint(point);
            }

            if (!bus.is_circular) {
                for (auto it = std::next(bus.stops.rbegin()); it != bus.stops.rend(); ++it) {
                    const auto& stop = tc.GetStops().at(std::string(*it));
                    auto point = projector(stop.coordinates);
                   polyline.AddPoint(point);
                }
            }

            doc.Add(std::move(polyline));
            ++color_index;
        }

        color_index = 0;
        for (const auto& name : bus_names) {
            const auto& bus = tc.GetBuses().at(name);
            if (bus.stops.empty()) continue;

            const auto& color = settings.color_palette[color_index % settings.color_palette.size()];
            const auto& first_stop = tc.GetStops().at(std::string(bus.stops.front())).coordinates;
            const auto& last_stop = tc.GetStops().at(std::string(bus.stops.back())).coordinates;

            auto draw_text = [&](const geo::Coordinates& coords, const std::string& label) {
                svg::Text text_underlayer;
                text_underlayer.SetPosition(projector(coords))
                    .SetOffset(settings.bus_label_offset)
                    .SetFontSize(settings.bus_label_font_size)
                    .SetFontFamily("Verdana")
                    .SetFontWeight("bold")
                    .SetData(label)
                    .SetFillColor(settings.underlayer_color)
                    .SetStrokeColor(settings.underlayer_color)
                    .SetStrokeWidth(settings.underlayer_width)
                    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

                svg::Text text;
                text.SetPosition(projector(coords))
                    .SetOffset(settings.bus_label_offset)
                    .SetFontSize(settings.bus_label_font_size)
                    .SetFontFamily("Verdana")
                    .SetFontWeight("bold")
                    .SetData(label)
                    .SetFillColor(color);

                doc.Add(std::move(text_underlayer));
                doc.Add(std::move(text));
                };

            draw_text(first_stop, bus.name);
            if (!bus.is_circular && bus.stops.front() != bus.stops.back()) {
                draw_text(last_stop, bus.name);
            }

            ++color_index;
        }

        std::vector<std::string> stop_names;
        for (const auto& [name, stop] : tc.GetStops()) {
            if (tc.GetBusesByStop(name).has_value()) {
                stop_names.push_back(name);
            }
        }
        std::sort(stop_names.begin(), stop_names.end());

        for (const auto& name : stop_names) {
            const auto& stop = tc.GetStops().at(name).coordinates;
            svg::Circle circle;
            circle.SetCenter(projector(stop))
                .SetRadius(settings.stop_radius)
                .SetFillColor("white");

            doc.Add(std::move(circle));
        }

        for (const auto& name : stop_names) {
            const auto& stop = tc.GetStops().at(name).coordinates;

            svg::Text text_underlayer;
            text_underlayer.SetPosition(projector(stop))
                .SetOffset(settings.stop_label_offset)
                .SetFontSize(settings.stop_label_font_size)
                .SetFontFamily("Verdana")
                .SetData(name)
                .SetFillColor(settings.underlayer_color)
                .SetStrokeColor(settings.underlayer_color)
                .SetStrokeWidth(settings.underlayer_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            svg::Text text;
            text.SetPosition(projector(stop))
                .SetOffset(settings.stop_label_offset)
                .SetFontSize(settings.stop_label_font_size)
                .SetFontFamily("Verdana")
                .SetData(name)
                .SetFillColor("black");

            doc.Add(std::move(text_underlayer));
            doc.Add(std::move(text));
        }

        doc.Render(output);
    }

} // namespace map_renderer
