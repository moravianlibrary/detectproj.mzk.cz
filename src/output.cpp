#include "output.h"

bool header_printed = false;

void print_header() {
    if (!header_printed) {
        FCGI_printf("Content-type: application/json\r\n");
        FCGI_printf("Access-Control-Allow-Origin: *\r\n\r\n");
        header_printed = true;
    }
}

void output_init() {
    header_printed = false;
}

void print(const std::string& msg) {
    print_header();
    FCGI_printf("{\"status\" : \"Done\", \"projections\": %s}", msg.c_str());
}

void print_processed() {
    print_header();
    FCGI_printf("{\"status\" : \"Processed\"}");
}

void print_error(const std::string& msg) {
    print_header();
    Json::Value root;
    root["status"] = Json::Value("Error");
    root["message"] = Json::Value(msg);
    Json::FastWriter writer;
    FCGI_printf(writer.write(root).c_str());
}

void print_detectproj_error(const std::string& msg) {
    print_header();
    Json::Value root;
    root["status"] = Json::Value("DetectprojError");
    root["message"] = Json::Value(msg);
    Json::FastWriter writer;
    FCGI_printf(writer.write(root).c_str());
}
