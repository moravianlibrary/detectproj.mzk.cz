#ifndef NO_FCGI_DEFINES
#define NO_FCGI_DEFINES
#endif


#include <cstdlib>
#include <map>
#include <sstream>
#include <limits>
#include <math.h>
#include <unistd.h>    /* define fork(), etc.   */
#include <sys/types.h> /* define pid_t, etc.    */
#include <sys/wait.h>  /* define wait(), etc.   */
#include <signal.h>    /* define signal(), etc. */
#include <assert.h>

#include "fcgi_stdio.h"
#include "jsoncpp/json/json.h"

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

void detectproj(const string& map);

bool run_detectproj(const string& map);

template<typename T> bool parse_points(const Value& json, vector<T*>& points, const string& key_x, const string& key_y);

void find_interest_points(vector<Node3DCartesian <double> *>& test_points, vector<Point3DGeographic <double> *>& ref_points, vector<Node3DCartesian <double> *>& out_test, vector<Point3DGeographic <double> *>& out_ref, int m, int n);

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
        } else if (sql_res == PROCESSED) {
            print_processed();
        } else if (sql_res == NOT_FOUND) {
            if (!run_detectproj(map)) {
                continue;
            }
            print_processed();
        } else if (sql_res == DETECTPROJ_ERROR) {
            print_detectproj_error(proj_res);
        } else {
            print_error(proj_res);
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
    for (int i = 0; i < control_points.size(); i++) {
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

void find_interest_points(vector<Node3DCartesian <double> *>& test_points, vector<Point3DGeographic <double> *>& ref_points, vector<Node3DCartesian <double> *>& out_test, vector<Point3DGeographic <double> *>& out_ref, int m, int n) {
    if (test_points.size() < m * n) {
        for (vector<Node3DCartesian <double> *>::iterator it = test_points.begin(); it != test_points.end(); it++) {
            out_test.push_back(*it);
            test_points.erase(it);
        }
        for (vector<Point3DGeographic <double> *>::iterator it = ref_points.begin(); it != ref_points.end(); it++) {
            out_ref.push_back(*it);
            ref_points.erase(it);
        }
        return;
    }

    // find bounding box
    double minx = numeric_limits<double>::max(), miny = numeric_limits<double>::max();
    double maxx = numeric_limits<double>::min(), maxy = numeric_limits<double>::min();
    for (vector<Node3DCartesian <double> *>::iterator it = test_points.begin(); it != test_points.end(); it++) {
        Node3DCartesian <double> * point = *it;
        if (point->getX() < minx) {
            minx = point->getX();
        }
        if (point->getX() > maxx) {
            maxx = point->getX();
        }
        if (point->getY() < miny) {
            miny = point->getY();
        }
        if (point->getY() > maxy) {
            maxy = point->getY();
        }
    }
    double w = abs(maxx - minx);
    double h = abs(maxy - miny);
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            double x = i*(w/m) + (w/(2*m));
            double y = j*(h/n) + (h/(2*n));
            double mindistance = numeric_limits<double>::max();
            vector<Node3DCartesian <double> *>::iterator nearest_point;
            for (vector<Node3DCartesian <double> *>::iterator it = test_points.begin(); it != test_points.end(); it++) {
                Node3DCartesian <double> * point = *it;
                double distance = sqrt(pow(x - point->getX(), 2) + pow(y - point->getY(), 2));
                if (distance < mindistance) {
                    mindistance = distance;
                    nearest_point = it;
                }
            }
            int nearest_point_index = nearest_point - test_points.begin();
            out_test.push_back(*nearest_point);
            out_ref.push_back(ref_points[nearest_point_index]);
            ref_points.erase(ref_points.begin() + nearest_point_index);
            test_points.erase(nearest_point);
        }
    }
    for (vector<Node3DCartesian <double> *>::iterator it = test_points.begin(); it != test_points.end();) {
        delete *it;
        it = test_points.erase(it);
    }
    for (vector<Point3DGeographic <double> *>::iterator it = ref_points.begin(); it != ref_points.end();) {
        delete *it;
        it = ref_points.erase(it);
    }
}

string get_json_uri(const string& map) {
    return "/map/" + map + "/json";
}

void detectproj(const string& map) {
    string json_uri;
    string json_data;
    // read JSON data
    json_uri = get_json_uri(map);
    if (!do_get("staremapy.georeferencer.cz", json_uri, json_data)) {
        return;
    }
    Value json;
    if (!parse_json(json_data, json)) {
        set_error(map, "Error at parsing JSON.");
        return;
    }
    vector<Node3DCartesian <double> *> test_points;
    vector<Point3DGeographic <double> *> ref_points;

    if (!parse_points<Node3DCartesian <double> >(json, test_points, TEST_POINT_X, TEST_POINT_Y)) {
        set_error(map, "Error at parsing JSON.");
        return;
    }
    if (!parse_points<Point3DGeographic <double> >(json, ref_points, REF_POINT_X, REF_POINT_Y)) {
        set_error(map, "Error at parsing JSON.");
        return;
    }
    for (vector<Node3DCartesian <double> *>::iterator it = test_points.begin(); it != test_points.end(); it++) {
        Node3DCartesian <double> * point = *it;
        point->setY(point->getY() * -1.0);
    }

    vector<Node3DCartesian <double> *> interest_test_points;
    vector<Point3DGeographic <double> *> interest_ref_points;

    find_interest_points(test_points, ref_points, interest_test_points, interest_ref_points, 3, 3);

    assert(interest_test_points.size() == interest_ref_points.size());
    assert(interest_test_points.size() == 9);
    assert(test_points.empty());
    assert(ref_points.empty());

    stringstream out;
    stringstream err;

    if (detectproj(interest_test_points, interest_ref_points, out, err)) {
        set_proj(map, out.str());
    } else {
        set_detectproj_error(map, err.str());
    }
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
