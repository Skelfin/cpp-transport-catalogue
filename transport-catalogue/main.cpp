#include "json_reader.h"
#include "transport_catalogue.h"
#include <iostream>

int main() {
    std::istream& input = std::cin;

    transport_catalogue::TransportCatalogue tc;

    json_reader::ProcessRequests(input, std::cout, tc);

    return 0;
}
