#include "svg.h"
#include <iomanip>

namespace svg {

    using namespace std::literals;

    std::ostream& operator<<(std::ostream& out, const StrokeLineCap& line_cap) {
        switch (line_cap) {
        case StrokeLineCap::BUTT:
            out << "butt";
            break;
        case StrokeLineCap::ROUND:
            out << "round";
            break;
        case StrokeLineCap::SQUARE:
            out << "square";
            break;
        }
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const StrokeLineJoin& line_join) {
        switch (line_join) {
        case StrokeLineJoin::ARCS:
            out << "arcs";
            break;
        case StrokeLineJoin::BEVEL:
            out << "bevel";
            break;
        case StrokeLineJoin::MITER:
            out << "miter";
            break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip";
            break;
        case StrokeLineJoin::ROUND:
            out << "round";
            break;
        }
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const Color& color) {
        std::visit([&out](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                out << "none";
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                out << value;
            }
            else if constexpr (std::is_same_v<T, Rgb>) {
                out << "rgb(" << static_cast<int>(value.red) << ","
                    << static_cast<int>(value.green) << ","
                    << static_cast<int>(value.blue) << ")";
            }
            else if constexpr (std::is_same_v<T, Rgba>) {
                out << "rgba(" << static_cast<int>(value.red) << ","
                    << static_cast<int>(value.green) << ","
                    << static_cast<int>(value.blue) << ","
                    << value.opacity << ")";
            }
            }, color);
        return out;
    }

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();
        RenderObject(context);
        context.out << std::endl;
    }

    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" ";
        out << "r=\""sv << radius_ << "\"";
        RenderAttrs(out);
        out << "/>"sv;
    }

    Polyline& Polyline::AddPoint(Point point) {
        points_.push_back(point);
        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<polyline points=\""sv;
        bool first = true;
        for (const auto& point : points_) {
            if (!first) {
                out << " "sv;
            }
            first = false;
            out << point.x << ","sv << point.y;
        }
        out << "\"";
        RenderAttrs(out);
        out << "/>"sv;
    }

    Text& Text::SetPosition(Point pos) {
        position_ = pos;
        return *this;
    }

    Text& Text::SetOffset(Point offset) {
        offset_ = offset;
        return *this;
    }

    Text& Text::SetFontSize(uint32_t size) {
        font_size_ = size;
        return *this;
    }

    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = std::move(font_family);
        return *this;
    }

    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = std::move(font_weight);
        return *this;
    }

    Text& Text::SetData(std::string data) {
        data_ = std::move(data);
        return *this;
    }

    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<text x=\""sv << position_.x << "\" y=\""sv << position_.y << "\" ";
        out << "dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" ";
        out << "font-size=\""sv << font_size_ << "\"";
        if (!font_family_.empty()) {
            out << " font-family=\""sv << font_family_ << "\"";
        }
        if (!font_weight_.empty()) {
            out << " font-weight=\""sv << font_weight_ << "\"";
        }
        RenderAttrs(out);
        out << ">"sv;
        out << EscapeText(data_);
        out << "</text>"sv;
    }

    void Document::AddPtr(std::unique_ptr<Object>&& obj) {
        objects_.emplace_back(std::move(obj));
    }

    void Document::Render(std::ostream& out) const {
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
        RenderContext ctx(out, 2, 2);
        for (const auto& obj : objects_) {
            obj->Render(ctx);
        }
        out << "</svg>"sv;
    }

    std::string EscapeText(const std::string& data) {
        std::string escaped;
        for (char c : data) {
            switch (c) {
            case '"':
                escaped += "&quot;"s;
                break;
            case '\'':
                escaped += "&apos;"s;
                break;
            case '<':
                escaped += "&lt;"s;
                break;
            case '>':
                escaped += "&gt;"s;
                break;
            case '&':
                escaped += "&amp;"s;
                break;
            default:
                escaped += c;
                break;
            }
        }
        return escaped;
    }

}  // namespace svg
