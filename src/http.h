#ifndef Http_H
#define Http_H

#include <string>
#include <map>

#include "output.h"

#include "uriparser/Uri.h"

#include <neon/ne_session.h>
#include <neon/ne_request.h>
#include <neon/ne_utils.h>
#include <neon/ne_uri.h>

bool parse_uri(const std::string& query, std::map<std::string, std::string>& params);

bool do_get(const std::string& host, const std::string& query, std::string& response);

#endif
