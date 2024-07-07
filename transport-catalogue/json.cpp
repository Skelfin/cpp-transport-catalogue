#include "json.h"
#include <sstream>
#include <stdexcept>
#include <cctype>

using namespace std;

namespace json {

    namespace {

        void SkipComment(istream& input) {
            char c;
            if (input.get(c)) {
                if (c == '/') {
                    if (input.peek() == '/') {
                        while (input.get(c) && c != '\n');
                    }
                    else if (input.peek() == '*') {
                        input.get(c);
                        while (input.get(c)) {
                            if (c == '*' && input.peek() == '/') {
                                input.get(c);
                                break;
                            }
                        }
                    }
                    else {
                        input.putback(c);
                    }
                }
                else {
                    input.putback(c);
                }
            }
        }

        Node LoadNode(istream& input);

        Node LoadArray(istream& input) {
            Array result;
            char c;
            while (input >> c) {
                if (c == ']') {
                    return Node(move(result));
                }
                else if (c == ',') {
                    continue;
                }
                else if (c == '/') {
                    SkipComment(input);
                    continue;
                }
                else {
                    input.putback(c);
                    result.push_back(LoadNode(input));
                }
            }
            throw ParsingError("Expected ']' at the end of array");
        }

        Node LoadIntOrDouble(istream& input) {
            string number;
            while (isdigit(input.peek()) || input.peek() == '.' || input.peek() == 'e' || input.peek() == 'E' || input.peek() == '-' || input.peek() == '+') {
                number += static_cast<char>(input.get());
            }
            if (number.empty() || (number == "-" || number == "+")) {
                throw ParsingError("Invalid number format");
            }
            if (number.find('.') != string::npos || number.find('e') != string::npos || number.find('E') != string::npos) {
                return Node(stod(number));
            }
            else {
                return Node(stoi(number));
            }
        }

        Node LoadString(istream& input) {
            string result;
            char c;
            while (input.get(c)) {
                if (c == '"') {
                    return Node(move(result));
                }
                if (c == '\\') {
                    if (!input.get(c)) {
                        throw ParsingError("Unexpected end of input in string escape sequence");
                    }
                    switch (c) {
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    default: throw ParsingError("Invalid escape sequence in string");
                    }
                }
                else {
                    result += c;
                }
            }
            throw ParsingError("Expected '\"' at the end of string");
        }

        Node LoadDict(istream& input) {
            Dict result;
            char c;
            while (input >> c) {
                if (c == '}') {
                    return Node(move(result));
                }
                else if (c == ',') {
                    continue;
                }
                else if (c == '/') {
                    SkipComment(input);
                    continue;
                }
                else if (c == '"') {
                    string key = LoadString(input).AsString();
                    input >> c;
                    if (c != ':') {
                        throw ParsingError("Expected ':' after key in dict");
                    }
                    result.insert({ move(key), LoadNode(input) });
                }
                else {
                    throw ParsingError("Expected '\"' at the beginning of key in dict");
                }
            }
            throw ParsingError("Expected '}' at the end of dict");
        }

        Node LoadBool(istream& input) {
            string result;
            while (isalpha(input.peek())) {
                result += static_cast<char>(input.get());
            }
            if (result == "true") return Node(true);
            if (result == "false") return Node(false);
            throw ParsingError("Invalid boolean value");
        }

        Node LoadNull(istream& input) {
            string result;
            while (isalpha(input.peek())) {
                result += static_cast<char>(input.get());
            }
            if (result == "null") return Node(nullptr);
            throw ParsingError("Invalid null value");
        }

        Node LoadNode(istream& input) {
            char c;
            while (input >> c) {
                if (c == '[') {
                    return LoadArray(input);
                }
                else if (c == '{') {
                    return LoadDict(input);
                }
                else if (c == '"') {
                    return LoadString(input);
                }
                else if (c == 't' || c == 'f') {
                    input.putback(c);
                    return LoadBool(input);
                }
                else if (c == 'n') {
                    input.putback(c);
                    return LoadNull(input);
                }
                else if (isdigit(c) || c == '-' || c == '+') {
                    input.putback(c);
                    return LoadIntOrDouble(input);
                }
                else if (c == '/') {
                    SkipComment(input);
                    continue;
                }
                else {
                    throw ParsingError("Unexpected character in input");
                }
            }
            throw ParsingError("Unexpected end of input");
        }

        struct PrintContext {
            std::ostream& out;
            int indent_step = 4;
            int indent = 0;

            void PrintIndent() const {
                for (int i = 0; i < indent; ++i) {
                    out.put(' ');
                }
            }

            PrintContext Indented() const {
                return { out, indent_step, indent + indent_step };
            }
        };

        void PrintNode(const Node& node, const PrintContext& ctx);

        void PrintValue(std::nullptr_t, const PrintContext& ctx) {
            ctx.out << "null";
        }

        void PrintValue(const Array& array, const PrintContext& ctx) {
            ctx.out << "[\n";
            auto inner_ctx = ctx.Indented();
            bool first = true;
            for (const auto& element : array) {
                if (!first) {
                    ctx.out << ",\n";
                }
                first = false;
                inner_ctx.PrintIndent();
                PrintNode(element, inner_ctx);
            }
            ctx.out << '\n';
            ctx.PrintIndent();
            ctx.out << ']';
        }

        void PrintValue(const Dict& dict, const PrintContext& ctx) {
            ctx.out << "{\n";
            auto inner_ctx = ctx.Indented();
            bool first = true;
            for (const auto& [key, value] : dict) {
                if (!first) {
                    ctx.out << ",\n";
                }
                first = false;
                inner_ctx.PrintIndent();
                ctx.out << '"' << key << "\": ";
                PrintNode(value, inner_ctx);
            }
            ctx.out << '\n';
            ctx.PrintIndent();
            ctx.out << '}';
        }

        void PrintValue(const std::string& value, const PrintContext& ctx) {
            ctx.out << '"';
            for (char c : value) {
                switch (c) {
                case '\n': ctx.out << "\\n"; break;
                case '\r': ctx.out << "\\r"; break;
                case '\t': ctx.out << "\\t"; break;
                case '"': ctx.out << "\\\""; break;
                case '\\': ctx.out << "\\\\"; break;
                default: ctx.out << c; break;
                }
            }
            ctx.out << '"';
        }

        void PrintValue(bool value, const PrintContext& ctx) {
            ctx.out << (value ? "true" : "false");
        }

        void PrintValue(int value, const PrintContext& ctx) {
            ctx.out << value;
        }

        void PrintValue(double value, const PrintContext& ctx) {
            ctx.out << value;
        }

        void PrintNode(const Node& node, const PrintContext& ctx) {
            std::visit([&ctx](const auto& value) { PrintValue(value, ctx); }, node.GetValue());
        }

    }  // namespace

    Document Load(istream& input) {
        return Document{ LoadNode(input) };
    }

    void Print(const Document& doc, ostream& output) {
        PrintContext ctx{ output };
        PrintNode(doc.GetRoot(), ctx);
    }

}  // namespace json