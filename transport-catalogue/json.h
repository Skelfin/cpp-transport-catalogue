#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>
#include <stdexcept>
#include <iostream>

namespace json {

    class ParsingError : public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };

    using Array = std::vector<class Node>;
    using Dict = std::map<std::string, class Node>;

    class Node : public std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string> {
    public:
        using variant::variant; // Inherit constructors

        bool IsInt() const { return std::holds_alternative<int>(*this); }
        bool IsDouble() const { return std::holds_alternative<double>(*this) || std::holds_alternative<int>(*this); }
        bool IsPureDouble() const { return std::holds_alternative<double>(*this); }
        bool IsBool() const { return std::holds_alternative<bool>(*this); }
        bool IsString() const { return std::holds_alternative<std::string>(*this); }
        bool IsNull() const { return std::holds_alternative<std::nullptr_t>(*this); }
        bool IsArray() const { return std::holds_alternative<Array>(*this); }
        bool IsMap() const { return std::holds_alternative<Dict>(*this); }

        int AsInt() const {
            if (!IsInt()) throw std::logic_error("Not an int");
            return std::get<int>(*this);
        }
        bool AsBool() const {
            if (!IsBool()) throw std::logic_error("Not a bool");
            return std::get<bool>(*this);
        }
        double AsDouble() const {
            if (IsInt()) return static_cast<double>(std::get<int>(*this));
            if (IsPureDouble()) return std::get<double>(*this);
            throw std::logic_error("Not a double");
        }
        const std::string& AsString() const {
            if (!IsString()) throw std::logic_error("Not a string");
            return std::get<std::string>(*this);
        }
        const Array& AsArray() const {
            if (!IsArray()) throw std::logic_error("Not an array");
            return std::get<Array>(*this);
        }
        const Dict& AsMap() const {
            if (!IsMap()) throw std::logic_error("Not a map");
            return std::get<Dict>(*this);
        }

        bool operator==(const Node& other) const {
            return static_cast<const variant&>(*this) == static_cast<const variant&>(other);
        }
        bool operator!=(const Node& other) const {
            return !(*this == other);
        }
    };

    class Document {
    public:
        explicit Document(Node root) : root_(std::move(root)) {}

        const Node& GetRoot() const { return root_; }

        bool operator==(const Document& other) const {
            return root_ == other.root_;
        }
        bool operator!=(const Document& other) const {
            return !(*this == other);
        }

    private:
        Node root_;
    };

    Document Load(std::istream& input);
    void Print(const Document& doc, std::ostream& output);

}  // namespace json
