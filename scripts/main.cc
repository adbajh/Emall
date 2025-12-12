#include <drogon/drogon.h>
#include "database/database.h"
#include "controllers/Controller.h"

using namespace drogon;

int main() {
    // 初始化数据文件夹
    initialize_file();
    // 初始化数据库
    initialize_db();
    //Set HTTP listener address and port
    drogon::app().addListener("0.0.0.0", 5555);
    drogon::app().setClientMaxBodySize(4 * 1024 * 1024); // max size: 4MB
    //Load config file
    //drogon::app().loadConfigFile("../config.json");
    //drogon::app().loadConfigFile("../config.yaml");
    //Run HTTP framework,the method will block in the internal event loop
    drogon::app().setDocumentRoot("scripts/public");
    drogon::app().run();
    return 0;
}
