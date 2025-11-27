#include "Controller.h"

// Add definition of your processing function here

void Controller::login_get(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    HttpViewData data;
    auto resp=HttpResponse::newHttpViewResponse("login.csp",data);
    callback(resp);
}

void Controller::register_get(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    HttpViewData data;
    auto resp=HttpResponse::newHttpViewResponse("_register.csp",data);
    callback(resp);
}

void Controller::index_get(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    HttpViewData data;
    auto resp=HttpResponse::newHttpViewResponse("home.csp",data);
    callback(resp);
}
