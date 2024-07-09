#include "json_reader.h"
#include "transport_catalogue.h"
#include "json.h"
#include <iostream>

int main() {
    transport_catalogue::TransportCatalogue tc;
    json_reader::JsonReader reader(tc);

    json::Document input_doc = json::Load(std::cin);

    json::Node output = reader.ProcessRequests(input_doc.GetRoot());

    json::Print(json::Document{ output }, std::cout);

    return 0;
}
