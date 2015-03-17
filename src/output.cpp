#include "output.h"

bool header_printed = false;

void print_header() {
    if (!header_printed) {
        FCGI_printf("Content-type: application/json\r\n\r\n");
        header_printed = true;
    }
}

void output_init() {
    header_printed = false;
}

void print(const std::string& msg) {
    print_header();
    FCGI_printf("{\"status\" : \"Ok\", \"geojson\": %s}", msg.c_str());
}

void print_wait() {
    print_header();
    FCGI_printf("{\"status\" : \"Wait\"}");
}

void print_error(const std::string& msg) {
    print_header();
    FCGI_printf("{\"status\" : \"Error\", \"message\" : \"%s\"}", msg.c_str());
}
