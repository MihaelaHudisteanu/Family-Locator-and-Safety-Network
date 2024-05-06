#include <iostream>
#include <sqlite3.h>

using namespace std;

int main() {
    sqlite3* db;

    if (sqlite3_open("database.db", &db)) 
    {
        cout << "[server]Database open error.\n";
        exit(EXIT_FAILURE);
    }

    // Query and print safe zones
    sqlite3_stmt* stmt;
    const char* selectQuery = "SELECT * FROM SafeZones;";

    sqlite3_prepare_v2(db, selectQuery, -1, &stmt, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int centerX = sqlite3_column_double(stmt, 1);
        int centerY = sqlite3_column_double(stmt, 2);
        int radius = sqlite3_column_double(stmt, 3);
        cout << "ID: " << id << ", Center: (" << centerX << ", " << centerY << "), Radius: " << radius << '\n';
    }

    sqlite3_finalize(stmt);


    sqlite3_prepare_v2(db, selectQuery, -1, &stmt, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int centerX = sqlite3_column_double(stmt, 1);
        int centerY = sqlite3_column_double(stmt, 2);
        int radius = sqlite3_column_double(stmt, 3);
        cout << "ID: " << id << ", Center: (" << centerX << ", " << centerY << "), Radius: " << radius << '\n';
    }

    sqlite3_finalize(stmt);

    sqlite3_close(db);

    return 0;
}
