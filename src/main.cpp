#ifndef NO_FCGI_DEFINES
#define NO_FCGI_DEFINES
#endif


#include <cstdlib>
#include <map>
#include <sstream>
#include <unistd.h>    /* define fork(), etc.   */
#include <sys/types.h> /* define pid_t, etc.    */
#include <sys/wait.h>  /* define wait(), etc.   */
#include <signal.h>    /* define signal(), etc. */

#include "fcgi_stdio.h"
#include "jsoncpp/json/reader.h"

#include "output.h"
#include "sqlite.h"
#include "detectproj.h"
#include "http.h"

using namespace std;
using namespace Json;

const string CONTROL_POINTS = "control_points";
const string REF_POINT_X = "latitude";
const string REF_POINT_Y = "longitude";
const string TEST_POINT_X = "pixel_x";
const string TEST_POINT_Y = "pixel_y";

string get_json_uri(const string& map);

bool parse_json(const string& data, Value& root);

bool detectproj(const string& map);

bool run_detectproj(const string& map);

template<typename T> bool parse_points(const Value& json, vector<T*>& points, const string& key_x, const string& key_y);

void catch_child(int sig_num);

int main(int argc, char** argv) {
    string error;
    bool sqlite_init_res;

    // register handler for ended child processes
    signal(SIGCHLD, catch_child);
    detectproj_init();
    sqlite_init_res = sqlite_init(error);

    while(FCGI_Accept() >= 0) {
        char* request_uri;
        map<string, string> params;
        string map;
        result_code sql_res;
        string proj_res;

        output_init();
        if (!sqlite_init_res) {
            print_error(error);
            continue;
        }
        // read GET URI
        request_uri = getenv("REQUEST_URI");
        if (!parse_uri(request_uri, params)) {
            continue;
        }
        if (!params.count("map")) {
            print_error("Parameter map must be presented.");
            continue;
        }
        map = params["map"];

        if (!get_proj(map, sql_res, proj_res)) {
            continue;
        }
        if (sql_res == DONE) {
            print(proj_res);
        } else if (sql_res == WAIT) {
            print_wait();
        } else if (sql_res == NOT_FOUND) {
            if (!run_detectproj(map)) {
                continue;
            }
            print_wait();
        } else {
            print_error("Wrong data.");
        }
    }
}

bool parse_json(const string& data, Value& root) {
    Reader reader;
    return reader.parse(data, root);
}

template<typename T> bool parse_points(const Value& json, vector<T*>& points, const string& key_x, const string& key_y) {
    if (!json.isMember(CONTROL_POINTS)) {
        print_error("You must specify " + CONTROL_POINTS);
        return false;
    }
    Value control_points = json[CONTROL_POINTS];
    if (!control_points.isArray()) {
        print_error(CONTROL_POINTS + " must be Array.");
        return false;
    }
    for (int i = 0; i < control_points.size() && i < 10; i++) {
        Value item = control_points[i];
        if (!item.isMember(key_x)) {
            print_error(key_x + " is not presented.");
            return false;
        }
        if (!item.isMember(key_y)) {
            print_error(key_y + " is not presented.");
            return false;
        }
        Value vx = item[key_x];
        Value vy = item[key_y];
        if (!vx.isDouble()) {
            print_error(key_x + " must be type of double.");
            return false;
        }
        if (!vy.isDouble()) {
            print_error(key_y + " must be type of double.");
            return false;
        }
        T* point = new T(vx.asDouble(), vy.asDouble());
        points.push_back(point);
    }
    return true;
}

string get_json_uri(const string& map) {
    return "/map/" + map + "/json";
}

bool detectproj(const string& map) {
    string json_uri;
    string json_data;
    // read JSON data
    json_uri = get_json_uri(map);
    if (!do_get("staremapy.georeferencer.cz", json_uri, json_data)) {
        return false;
    }
    Value json;
    if (!parse_json(json_data, json)) {
        print_error("Error at parsing JSON.");
        return false;
    }
    vector<Node3DCartesian <double> *> test_points;
    vector<Point3DGeographic <double> *> ref_points;

    if (!parse_points<Node3DCartesian <double> >(json, test_points, TEST_POINT_X, TEST_POINT_Y)) {
        return false;
    }
    if (!parse_points<Point3DGeographic <double> >(json, ref_points, REF_POINT_X, REF_POINT_Y)) {
        return false;
    }
    for (vector<Node3DCartesian <double> *>::iterator it = test_points.begin(); it != test_points.end(); it++) {
        Node3DCartesian <double> * point = *it;
        point->setY(point->getY() * -1.0);
    }
    stringstream ss;
    detectproj(test_points, ref_points, ss);
    set_proj(map, ss.str());
    return true;
}

bool run_detectproj(const string& map) {
    pid_t pid = fork();

    if (pid == 0) {
        detectproj(map);
        exit(0);
    } else if (pid > 0) {
        return true;
    } else {
        print_error("Fork failed.");
        return false;
    }
}

void catch_child(int sig_num) {
    /* when we get here, we know there's a zombie child waiting */
    int child_status;
    wait(&child_status);
}
