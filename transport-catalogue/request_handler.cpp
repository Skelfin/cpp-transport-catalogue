#include "request_handler.h"
#include <stdexcept>

namespace request_handler {

    RequestHandler::RequestHandler(const transport_catalogue::TransportCatalogue& db)
        : db_(db) {}

    std::optional<std::vector<std::string>> RequestHandler::GetBusesByStop(const std::string& stop_name) const {
        return db_.GetBusesByStop(stop_name);
    }

    domain::BusInfo RequestHandler::GetBusInfo(const std::string& bus_name) const {
        return db_.GetBusInfo(bus_name);
    }

} // namespace request_handler
