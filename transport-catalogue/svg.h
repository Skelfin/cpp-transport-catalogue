#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace svg {

    struct Rgb {
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;

        Rgb() = default;
        Rgb(uint8_t r, uint8_t g, uint8_t b) : red(r), green(g), blue(b) {}
    };

    struct Rgba {
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
        double opacity = 1.0;

        Rgba() = default;
        Rgba(uint8_t r, uint8_t g, uint8_t b, double o) : red(r), green(g), blue(b), opacity(o) {}
    };

    using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

    inline const Color NoneColor{ std::monostate{} };

    enum class StrokeLineCap {
        BUTT,
        ROUND,
        SQUARE,
    };

    enum class StrokeLineJoin {
        ARCS,
        BEVEL,
        MITER,
        MITER_CLIP,
        ROUND,
    };

    std::ostream& operator<<(std::ostream& out, const StrokeLineCap& line_cap);
    std::ostream& operator<<(std::ostream& out, const StrokeLineJoin& line_join);
    std::ostream& operator<<(std::ostream& out, const Color& color);

    struct Point {
        Point() = default;
        Point(double x, double y) : x(x), y(y) {}
        double x = 0;
        double y = 0;
    };

    struct RenderContext {
        RenderContext(std::ostream& out) : out(out) {}

        RenderContext(std::ostream& out, int indent_step, int indent = 0)
            : out(out), indent_step(indent_step), indent(indent) {}

        RenderContext Indented() const {
            return { out, indent_step, indent + indent_step };
        }

        void RenderIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        std::ostream& out;
        int indent_step = 0;
        int indent = 0;
    };

    class Object {
    public:
        void Render(const RenderContext& context) const;

        virtual ~Object() = default;

    private:
        virtual void RenderObject(const RenderContext& context) const = 0;
    };

    template <typename Derived>
    class PathProps {
    public:
        Derived& SetFillColor(Color color) {
            fill_color_ = std::move(color);
            fill_color_is_set_ = true;
            return AsDerived();
        }

        Derived& SetStrokeColor(Color color) {
            stroke_color_ = std::move(color);
            return AsDerived();
        }

        Derived& SetStrokeWidth(double width) {
            stroke_width_ = width;
            return AsDerived();
        }

        Derived& SetStrokeLineCap(StrokeLineCap line_cap) {
            stroke_linecap_ = line_cap;
            return AsDerived();
        }

        Derived& SetStrokeLineJoin(StrokeLineJoin line_join) {
            stroke_linejoin_ = line_join;
            return AsDerived();
        }

    protected:
        void RenderAttrs(std::ostream& out) const {
            if (fill_color_is_set_) {
                out << " fill=\"" << fill_color_ << "\"";
            }

            if (!std::holds_alternative<std::monostate>(stroke_color_)) {
                out << " stroke=\"" << stroke_color_ << "\"";
                if (stroke_width_ != 1.0) {
                    out << " stroke-width=\"" << stroke_width_ << "\"";
                }
                if (stroke_linecap_ != StrokeLineCap::BUTT) {
                    out << " stroke-linecap=\"" << stroke_linecap_ << "\"";
                }
                if (stroke_linejoin_ != StrokeLineJoin::MITER) {
                    out << " stroke-linejoin=\"" << stroke_linejoin_ << "\"";
                }
            }
        }

    private:
        Derived& AsDerived() {
            return static_cast<Derived&>(*this);
        }

        Color fill_color_ = std::monostate{};
        Color stroke_color_ = std::monostate{};
        double stroke_width_ = 1.0;
        StrokeLineCap stroke_linecap_ = StrokeLineCap::BUTT;
        StrokeLineJoin stroke_linejoin_ = StrokeLineJoin::MITER;
        bool fill_color_is_set_ = false;
    };

    class Circle final : public Object, public PathProps<Circle> {
    public:
        Circle& SetCenter(Point center);
        Circle& SetRadius(double radius);

    private:
        void RenderObject(const RenderContext& context) const override;

        Point center_;
        double radius_ = 1.0;
    };

    class Polyline final : public Object, public PathProps<Polyline> {
    public:
        Polyline& AddPoint(Point point);

    private:
        void RenderObject(const RenderContext& context) const override;

        std::vector<Point> points_;
    };

    class Text final : public Object, public PathProps<Text> {
    public:
        Text& SetPosition(Point pos);
        Text& SetOffset(Point offset);
        Text& SetFontSize(uint32_t size);
        Text& SetFontFamily(std::string font_family);
        Text& SetFontWeight(std::string font_weight);
        Text& SetData(std::string data);

    private:
        void RenderObject(const RenderContext& context) const override;

        Point position_ = { 0, 0 };
        Point offset_ = { 0, 0 };
        uint32_t font_size_ = 1;
        std::string font_family_;
        std::string font_weight_;
        std::string data_;
    };

    class ObjectContainer {
    public:
        template <typename Obj>
        void Add(Obj obj) {
            AddPtr(std::make_unique<Obj>(std::move(obj)));
        }

        virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

        virtual ~ObjectContainer() = default;
    };

    class Document : public ObjectContainer {
    public:
        void AddPtr(std::unique_ptr<Object>&& obj) override;

        void Render(std::ostream& out) const;

    private:
        std::vector<std::unique_ptr<Object>> objects_;
    };

    class Drawable {
    public:
        virtual void Draw(ObjectContainer& container) const = 0;

        virtual ~Drawable() = default;
    };

    std::string EscapeText(const std::string& data);

}  // namespace svg
