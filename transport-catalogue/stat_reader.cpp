#include "stat_reader.h"
#include <iomanip>
#include <iostream>

namespace transport_catalogue {

    void ParseAndPrintStat(const TransportCatalogue& transport_catalogue, std::string_view request, std::ostream& output) {
        if (request.find("Bus") != std::string::npos) {
            std::string bus_name = std::string(request.substr(4));
            try {
                auto [stop_count, unique_stop_count, route_length] = transport_catalogue.GetBusInfo(bus_name);
                output << "Bus " << bus_name << ": " << stop_count << " stops on route, "
                    << unique_stop_count << " unique stops, " << route_length << " route length\n";
            }
            catch (const std::out_of_range&) {
                output << "Bus " << bus_name << ": not found\n";
            }
        }
        else if (request.find("Stop") != std::string::npos) {
            std::string stop_name = std::string(request.substr(5));
            try {
                auto buses = transport_catalogue.GetBusesByStop(stop_name);
                if (buses.empty()) {
                    output << "Stop " << stop_name << ": no buses\n";
                }
                else {
                    output << "Stop " << stop_name << ": buses";
                    for (const auto& bus : buses) {
                        output << " " << bus;
                    }
                    output << '\n';
                }
            }
            catch (const std::out_of_range&) {
                output << "Stop " << stop_name << ": not found\n";
            }
        }
    }

}