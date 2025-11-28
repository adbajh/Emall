#pragma once

#include <drogon/HttpController.h>
#include <string_view>
#include "../database/database.h"

using namespace drogon;

#define ENABLE_LOGIN_REGISTER_LOGS
// #define WITHOUT_LOGIN_LOGS
// #define ENABLE_MESSAGE_LOGS

int idx_to_id(int idx, const string& type);
int id_to_idx(int id, const string& type);
void save_image(const HttpRequestPtr &req, const string &type, int idx);
string load_image(const string &type, int idx);
void delete_image(const string &type, int idx);
void initialize_file();

class Controller : public drogon::HttpController<Controller>
{
  public:
    METHOD_LIST_BEGIN
    // use METHOD_ADD to add your custom processing function here;
    // METHOD_ADD(Controller::get, "/{2}/{1}", Get); // path is /Controller/{arg2}/{arg1}
    // METHOD_ADD(Controller::your_method_name, "/{1}/{2}/list", Get); // path is /Controller/{arg1}/{arg2}/list
    // ADD_METHOD_TO(Controller::your_method_name, "/absolute/path/{1}/{2}/list", Get); // path is /absolute/path/{arg1}/{arg2}/list
    // page get for web app
    ADD_METHOD_TO(Controller::login_get,"/emall/login",Get);
    ADD_METHOD_TO(Controller::register_get,"/emall/register",Get);
    ADD_METHOD_TO(Controller::index_get,"/emall",Get);
    ADD_METHOD_TO(Controller::index_get,"/emall/search",Get);
    ADD_METHOD_TO(Controller::index_get,"/emall/item?idx={}",Get);
    ADD_METHOD_TO(Controller::index_get,"/emall/shop?idx={}",Get);
    ADD_METHOD_TO(Controller::index_get,"/emall/user?idx={}",Get);
    ADD_METHOD_TO(Controller::index_get,"/emall/manage", Get);
    ADD_METHOD_TO(Controller::index_get,"/emall/manage/join", Get);
    ADD_METHOD_TO(Controller::index_get,"/emall/manage/create", Get);
    ADD_METHOD_TO(Controller::index_get,"/emall/manage/shop/{}", Get);
    ADD_METHOD_TO(Controller::index_get,"/emall/manage/shop/{}/publish", Get);
    ADD_METHOD_TO(Controller::index_get,"/emall/manage/shop/{}/item/{}", Get);
    ADD_METHOD_TO(Controller::index_get,"/emall/temporary/user?idx={}", Get);
    ADD_METHOD_TO(Controller::index_get,"/emall/messages", Get);
    ADD_METHOD_TO(Controller::index_get,"/emall/setting", Get);
    ADD_METHOD_TO(Controller::index_get,"/emall/orders", Get);
    // login and register
    ADD_METHOD_TO(Controller::login_post,"/emall/login",Post);
    ADD_METHOD_TO(Controller::register_post,"/emall/register",Post);
    // require safe information
    ADD_METHOD_TO(Controller::require_safe,"/emall/require_safe?info={}&idx={}",Get);
    ADD_METHOD_TO(Controller::require_image,"/emall/require_image?info={}&idx={}",Get);
    // user related
    ADD_METHOD_TO(Controller::get_user_idx,"/emall/get_user_idx",Get);
    ADD_METHOD_TO(Controller::user_edit_info,"/emall/setting/edit",Post,"LoginFilter");
    ADD_METHOD_TO(Controller::user_edit_account,"/emall/setting/edit_account",Post,"LoginFilter");
    ADD_METHOD_TO(Controller::user_edit_password,"/emall/setting/edit_password",Post,"LoginFilter");
    ADD_METHOD_TO(Controller::logout,"/emall/setting/logout",Post,"LoginFilter");
    ADD_METHOD_TO(Controller::delete_account,"/emall/setting/delete",Delete,"LoginFilter");
    // shop related
    ADD_METHOD_TO(Controller::get_shop_idx,"/emall/manage/get_shop_idx",Get,"LoginFilter");
    ADD_METHOD_TO(Controller::setup_shop,"/emall/manage/create",Post,"LoginFilter");
    ADD_METHOD_TO(Controller::edit_shop,"/emall/manage/shop/{1}/edit",Post,"LoginFilter");
    ADD_METHOD_TO(Controller::dismiss_shop,"/emall/manage/shop/{1}/dismiss",Delete,"LoginFilter");
    ADD_METHOD_TO(Controller::find_shop_by_invite_code,"/emall/manage/join",Post,"LoginFilter");
    ADD_METHOD_TO(Controller::join_shop,"/emall/manage/confirm_join",Post,"LoginFilter");
    ADD_METHOD_TO(Controller::quit_shop,"/emall/manage/shop/{1}/quit",Delete,"LoginFilter");
    // item related
    ADD_METHOD_TO(Controller::search_next_item,"/emall/search_next_item",Post);
    ADD_METHOD_TO(Controller::get_item_idx,"/emall/manage/shop/{1}/get_item_idx",Get,"LoginFilter");
    ADD_METHOD_TO(Controller::publish_new_item,"/emall/manage/shop/{1}/publish",Post,"LoginFilter");
    ADD_METHOD_TO(Controller::edit_current_item,"/emall/manage/shop/{1}/item/{2}/edit",Post,"LoginFilter");
    ADD_METHOD_TO(Controller::item_offshelf,"/emall/manage/shop/{1}/item/{2}/offshelf",Delete,"LoginFilter");
    // order related
    ADD_METHOD_TO(Controller::initialize_order,"/emall/orders/create?item_idx={1}",Post,"LoginFilter");
    ADD_METHOD_TO(Controller::acquire_order,"/emall/orders/get",Post,"LoginFilter");
    ADD_METHOD_TO(Controller::modify_order,"/emall/orders/edit?idx={1}",Post,"LoginFilter");
    // message related
    ADD_METHOD_TO(Controller::get_history_liaison,"/emall/messages/get_history_liaison",Get,"LoginFilter");
    ADD_METHOD_TO(Controller::send_message,"/emall/messages/send",Post,"LoginFilter");
    ADD_METHOD_TO(Controller::receive_latest,"/emall/messages/receive_latest",Post,"LoginFilter");
    ADD_METHOD_TO(Controller::receive_history,"/emall/messages/receive_history",Post,"LoginFilter");
    ADD_METHOD_TO(Controller::mark_read,"/emall/messages/mark_read",Post,"LoginFilter");
    METHOD_LIST_END
    // your declaration of processing function maybe like this:
    // void get(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int p1, std::string p2);
    // void your_method_name(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, double p1, int p2) const;
    // page get for web app
    void login_get(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    void register_get(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    void index_get(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    // login and register
    void login_post(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    void register_post(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    // require safe information
    void require_safe(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& info, const string& idx) const;
    void require_image(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& info, const string& idx) const;
    // user related
    void get_user_idx(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    void user_edit_info(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    void user_edit_account(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    void user_edit_password(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    void logout(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    void delete_account(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    // shop related
    void get_shop_idx(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    void setup_shop(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    void edit_shop(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& shop_idx) const;
    void dismiss_shop(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& shop_idx) const;
    void find_shop_by_invite_code(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    void join_shop(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    void quit_shop(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& shop_idx) const;
    // item related
    void search_next_item(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    void get_item_idx(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& shop_idx) const;
    void publish_new_item(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& shop_idx) const;
    void edit_current_item(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& shop_idx, const string& item_idx) const;
    void item_offshelf(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& shop_idx, const string& item_idx) const;
    // order related
    void initialize_order(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& item_idx) const;
    void acquire_order(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    void modify_order(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& order_idx) const;
    // message related
    void get_history_liaison(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    void send_message(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    void receive_latest(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    void receive_history(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
    void mark_read(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const;
};

#ifdef ENABLE_LOGIN_REGISTER_LOGS

#define ENABLE_LOGIN_POST
#define ENABLE_REGISTER_POST

#endif

#ifdef WITHOUT_LOGIN_LOGS

#define ENABLE_GET_USER_IDX
#define ENABLE_SEARCH_NEXT_ITEM
#define ENABLE_REQUIRE_SAFE_TYPE
#define ENABLE_REQUIRE_SAFE_USER
#define ENABLE_REQUIRE_SAFE_ITEM
#define ENABLE_REQUIRE_SAFE_SHOP
#define ENABLE_REQUIRE_IMAGE

#endif


#ifdef ENABLE_MESSAGE_LOGS

#define ENABLE_SEND_MESSAGE
#define ENABLE_RECEIVE_LATEST
#define ENABLE_RECEIVE_HISTORY
#define ENABLE_MARK_READ

#endif
