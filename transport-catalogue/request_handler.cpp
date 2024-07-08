#include "request_handler.h"
#include <stdexcept>

namespace request_handler {

    RequestHandler::RequestHandler(const transport_catalogue::TransportCatalogue& db)
        : db_(db) {}

    std::optional<std::vector<BusDetails>> RequestHandler::GetBusesByStop(const std::string& stop_name) const {
        auto buses_opt = db_.GetBusesByStop(stop_name);
        if (!buses_opt) {
            return std::nullopt;
        }

        std::vector<BusDetails> bus_details;
        for (const auto& bus_name : *buses_opt) {
            bus_details.push_back({ bus_name });
        }

        return bus_details;
    }

    domain::BusInfo RequestHandler::GetBusInfo(const std::string& bus_name) const {
        return db_.GetBusInfo(bus_name);
    }

} // namespace request_handler
