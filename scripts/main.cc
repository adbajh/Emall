#include <drogon/drogon.h>
#include "database/database.h"
#include "controllers/Controller.h"

using namespace drogon;

int main() {
    initialize_db();
    initialize_file();
    // initialize_database(database_name);
    // initialize_case(database_name);
    //Set HTTP listener address and port
    drogon::app().addListener("0.0.0.0", 5555);
    drogon::app().setClientMaxBodySize(4 * 1024 * 1024); // max size: 4MB
    //Load config file
    //drogon::app().loadConfigFile("../config.json");
    //drogon::app().loadConfigFile("../config.yaml");
    //Run HTTP framework,the method will block in the internal event loop
    drogon::app().setDocumentRoot("/home/amax/emall/scripts/public");
    drogon::app().run();
    return 0;
}
