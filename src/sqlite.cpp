#include "sqlite.h"

sqlite3 *db = NULL;

static int busy_callback(void* data, int count) {
    usleep(200000);
    return 1;
}

bool sqlite_init(std::string& error) {
    char *zErrMsg = 0;
    int  rc;
    std::string sql;

    /* Open database */
    rc = sqlite3_open(getenv("DETECTPROJ_DB"), &db);
    if (rc) {
        error = std::string(sqlite3_errmsg(db));
        return false;
    }
    sqlite3_busy_handler(db, busy_callback, NULL);
    /* Create SQL statement */
    sql = "CREATE TABLE IF NOT EXISTS detectproj("  \
          "map TEXT PRIMARY KEY   NOT NULL," \
          "status         TEXT," \
          "geojson        TEXT);";

    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql.c_str(), NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        error = std::string(sqlite3_errmsg(db));
        return false;
    }
    sql = "DELETE FROM detectproj WHERE status = 'processed'";
    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql.c_str(), NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        error = std::string(sqlite3_errmsg(db));
        return false;
    }
    return true;
}

static bool get_column_value(const std::string& id, const std::string& column, std::string& value) {
    char *zErrMsg = 0;
    int rc;
    std::string sql;
    sqlite3_stmt* stmt;
    const char* pzTest;

    sql = "SELECT " + column + " FROM detectproj WHERE map = ?";
    rc = sqlite3_prepare(db, sql.c_str(), sql.size(), &stmt, &pzTest);
    if (rc != SQLITE_OK) {
        print_error(std::string(sqlite3_errmsg(db)));
        return false;
    }
    sqlite3_bind_text(stmt, 1, id.c_str(), id.size(), NULL);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const char* sql_res = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        value = std::string(sql_res);
    } else if (rc == SQLITE_DONE) {
        // do nothing
    } else {
        sqlite3_finalize(stmt);
        print_error("SQL error: select");
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}

static bool set_processed_flag(const std::string& id) {
    char *zErrMsg = 0;
    int rc;
    std::string sql;
    sqlite3_stmt* stmt;

    sql = "INSERT OR REPLACE INTO detectproj(map, status) VALUES (?, 'processed')";
    rc = sqlite3_prepare(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        print_error(std::string(sqlite3_errmsg(db)));
        return false;
    }
    sqlite3_bind_text(stmt, 1, id.c_str(), id.size(), NULL);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        print_error(std::string(sqlite3_errmsg(db)));
        return false;
    }
    return true;
}

bool get_proj(const std::string& id, result_code& result_code, std::string& result) {
    char *zErrMsg = 0;
    int rc;
    std::string sql;
    std::string status = "not_found";

    if (!get_column_value(id, "status", status)) {
        return false;
    }

    if (status == "processed") {
        result_code = PROCESSED;
    } else if (status == "error") {
        if (!get_column_value(id, "geojson", result)) {
            return false;
        }
        result_code = ERROR;
    } else if (status == "detectproj_error") {
        if (!get_column_value(id, "geojson", result)) {
            return false;
        }
        result_code = DETECTPROJ_ERROR;
    } else if (status == "done") {
        if (!get_column_value(id, "geojson", result)) {
            return false;
        }
        result_code = DONE;
    } else if (status == "not_found") {
        sql = "BEGIN EXCLUSIVE;";
        sqlite3_exec(db, sql.c_str(), NULL, 0, &zErrMsg);

        if (!get_column_value(id, "status", status)) {
            sql = "ROLLBACK;";
            sqlite3_exec(db, sql.c_str(), NULL, 0, &zErrMsg);
            return false;
        }
        if (status == "not_found") {
            if (!set_processed_flag(id)) {
                sql = "ROLLBACK;";
                sqlite3_exec(db, sql.c_str(), NULL, 0, &zErrMsg);
                return false;
            }
        } else {
            sql = "END;";
            sqlite3_exec(db, sql.c_str(), NULL, 0, &zErrMsg);
            result_code = PROCESSED;
            return true;
        }
        sql = "COMMIT;";
        sqlite3_exec(db, sql.c_str(), NULL, 0, &zErrMsg);
        result_code = NOT_FOUND;
    } else {
        print_error("Unexpected value in status column.");
        return false;
    }
    return true;
}

bool set_proj(const std::string& id, const std::string& value) {
    char *zErrMsg = 0;
    int rc;
    std::string sql;
    sqlite3_stmt* stmt;

    sql = "INSERT OR REPLACE INTO detectproj(map, status, geojson) VALUES (?, 'done', ?)";
    rc = sqlite3_prepare(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        print_error(std::string(sqlite3_errmsg(db)));
        return false;
    }
    sqlite3_bind_text(stmt, 1, id.c_str(), id.size(), NULL);
    sqlite3_bind_text(stmt, 2, value.c_str(), value.size(), NULL);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        print_error(std::string(sqlite3_errmsg(db)));
        return false;
    }
    return true;
}

bool set_error(const std::string& id, const std::string& value) {
    char *zErrMsg = 0;
    int rc;
    std::string sql;
    sqlite3_stmt* stmt;

    sql = "INSERT OR REPLACE INTO detectproj(map, status, geojson) VALUES (?, 'error', ?)";
    rc = sqlite3_prepare(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        print_error(std::string(sqlite3_errmsg(db)));
        return false;
    }
    sqlite3_bind_text(stmt, 1, id.c_str(), id.size(), NULL);
    sqlite3_bind_text(stmt, 2, value.c_str(), value.size(), NULL);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        print_error(std::string(sqlite3_errmsg(db)));
        return false;
    }
    return true;
}

bool set_detectproj_error(const std::string& id, const std::string& value) {
    char *zErrMsg = 0;
    int rc;
    std::string sql;
    sqlite3_stmt* stmt;

    sql = "INSERT OR REPLACE INTO detectproj(map, status, geojson) VALUES (?, 'detectproj_error', ?)";
    rc = sqlite3_prepare(db, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        print_error(std::string(sqlite3_errmsg(db)));
        return false;
    }
    sqlite3_bind_text(stmt, 1, id.c_str(), id.size(), NULL);
    sqlite3_bind_text(stmt, 2, value.c_str(), value.size(), NULL);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        print_error(std::string(sqlite3_errmsg(db)));
        return false;
    }
    return true;
}
