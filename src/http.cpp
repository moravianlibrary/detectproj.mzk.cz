#include "http.h"
#include <sstream>

bool parse_uri(const std::string& query, std::map<std::string, std::string>& params) {
    UriParserStateA state;
    UriUriA uri;
    UriQueryListA * queryList;
    int itemCount;
    state.uri = &uri;
    if (uriParseUriA(&state, query.c_str()) != URI_SUCCESS) {
        /* Failure */
        uriFreeUriMembersA(&uri);
        print_error("Error at parsing URI");
        return false;
    }

    if (uriDissectQueryMallocA(&queryList, &itemCount, uri.query.first,
                        uri.query.afterLast) != URI_SUCCESS) {
        /* Failure */
        print_error("Error at parsing URI");
        return false;
    }
    UriQueryListA * tmp;
    while (queryList) {
        params[std::string(queryList->key)] = std::string(queryList->value);
        tmp = queryList;
        queryList = queryList->next;
        free((char*)tmp->key);
        free((char*)tmp->value);
        free(tmp);
    }

    uriFreeUriMembersA(&uri);
    return true;
}

int httpResponseReader(void *userdata, const char *buf, size_t len) {
    std::string *str = (std::string *)userdata;
    str->append(buf, len);
    return 0;
}

bool do_get(const std::string& host, const std::string& query, std::string& response) {
    ne_session *sess;
    ne_request *req;

    ne_sock_init();

    sess = ne_session_create("http", host.c_str(), 80);
    ne_set_useragent(sess, "MyAgent/1.0");

    req = ne_request_create(sess, "GET", query.c_str());
    // if accepting only 2xx codes, use "ne_accept_2xx"
    ne_add_response_body_reader(req, ne_accept_always, httpResponseReader, &response);

    int result = ne_request_dispatch(req);
    int status = ne_get_status(req)->code;

    ne_request_destroy(req);

    std::string error_message = ne_get_error(sess);
    ne_session_destroy(sess);
    bool error = false;
    std::string error_type;

    switch (result) {
    case NE_OK:
        break;
    case NE_CONNECT:
        error = true;
        error_type = "Connection Error";
        break;
    case NE_TIMEOUT:
        error = true;
        error_type = "Timeout Error";
        break;
    case NE_AUTH:
        error = true;
        error_type = "Authentication Error";
        break;
    default:
        error = true;
        error_type = "Nonspecified Error";
        break;
    }
    if (error) {
        std::stringstream ss;
        ss << error_type << " " << error_message;
        print_error(ss.str());
        return false;
    }

    return true;
}
