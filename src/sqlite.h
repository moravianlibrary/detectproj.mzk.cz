#ifndef SQLITE_H_
#define SQLITE_H_

#include <string>
#include <iostream>
#include <sqlite3.h>
#include <unistd.h>
#include <stdlib.h>

#include "output.h"

enum result_code {
    ERROR, DETECTPROJ_ERROR, DONE, PROCESSED, NOT_FOUND
};

extern sqlite3 *db;

bool sqlite_init(std::string& error);

bool get_proj(const std::string& id, result_code& result_code, std::string& result);

bool set_proj(const std::string& id, const std::string& value);

bool set_error(const std::string& id, const std::string& value);

bool set_detectproj_error(const std::string& id, const std::string& value);

#endif
