#ifndef Output_H
#define Output_H

#ifndef NO_FCGI_DEFINES
#define NO_FCGI_DEFINES
#endif

#include <string>
#include "fcgi_stdio.h"
#include "jsoncpp/json/json.h"

void output_init();

void print(const std::string& msg);

void print_processed();

void print_error(const std::string& msg);

#endif
