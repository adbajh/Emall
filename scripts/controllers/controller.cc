#include "Controller.h"
#include <fstream>

// Add definition of your processing function here

void Controller::login_post(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    auto json = req->getJsonObject();
    string account, password;
    if (json) {
        account = (*json)["account"].asString();
        password = (*json)["password"].asString();
    }
    #ifdef ENABLE_LOGIN_POST
    cout << endl;
    cout <<"[info]: User "<< account << " login" << endl;
    #endif
    string token;
    Json::Value ret;
    if (user_login(account, password, token) == 0) {
        #ifdef ENABLE_LOGIN_POST
        cout << "login success" << endl;
        cout << "token = " << token << endl;
        #endif
        ret["status"] = 0;
        ret["token"] = token;
        auto resp=HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    } else {
        #ifdef ENABLE_LOGIN_POST
        cout << "login failed" << endl;
        #endif
        ret["status"] = 1;
        ret["token"] = "";
        auto resp=HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    }
}

void Controller::register_post(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    auto json = req->getJsonObject();
    string name, account, password;
    if(json) {
        name = (*json)["name"].asString();
        account = (*json)["account"].asString();
        password = (*json)["password"].asString();
    }
    #ifdef ENABLE_REGISTER_POST
    cout << endl;
    cout << "[info]: registering " << name << " " << account << " "<< password;
    #endif
    Json::Value ret;
    if (user_register(name, account, password) == 0) {
        #ifdef ENABLE_REGISTER_POST
        cout << "register success" << endl;
        #endif
        ret["result"] = true;
    }
    else {
        #ifdef ENABLE_REGISTER_POST
        cout << "register failed" << endl;
        #endif
        ret["success"] = false;
        ret["error"] = "Account already exists";
        ret["result"] = false;
    }
    auto resp=HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::get_user_idx(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    string account = req->getHeader("UserAccount");
    string token = req->getHeader("UserToken");
    Json::Value ret;
    int user_idx = id_to_idx(user_get_id(account, token), "user");
    #ifdef ENABLE_GET_USER_IDX
    cout << endl;
    cout << "[info]: getting user idx" << endl;
    cout << "user_idx = " << user_idx << endl;
    #endif
    ret["user_idx"] = user_idx;
    auto resp=HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::search_next_item(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    auto json = req->getJsonObject();
    string cur_idx;
    map<string, string> cond;
    if (json) {
        const auto& keys = json->getMemberNames();
        for (const auto &key : keys) {
            cond[key] = (*json)[key].asString();
        }
        cur_idx = (*json)["current_idx"].asString();
    }
    if (cond.count("seller_idx")) {
        cond["seller_id"] = to_string(idx_to_id(stoi(cond["seller_idx"]), "user"));
    }
    if (cond.count("shop_idx")) {
        cond["shop_id"] = to_string(idx_to_id(stoi(cond["shop_idx"]), "shop"));
    }
    int id = search_next_item_id(idx_to_id(stoi(cur_idx), "item"), cond);

    #ifdef ENABLE_SEARCH_NEXT_ITEM
    cout << endl;
    cout << "[info]: searching next item from idx " << cur_idx << endl;
    cout << "condition:" << endl;
    for (const auto& [key, value] : cond) {
        cout << key << ": " << cond[key] << endl;
    }
    cout << "next_idx: " << id_to_idx(id, "item") << endl;
    #endif
    
    Json::Value ret;
    ret["next_idx"] = id_to_idx(id, "item");
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::require_safe(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& info, const string& idx) const {
    int i = std::stoi(idx);
    if (info == "type") {
        vector<string> types;
        get_category_information(types);
        Json::Value typesArray(Json::arrayValue);
        #ifdef ENABLE_REQUIRE_SAFE_TYPE
        cout << endl;
        cout << "[info]: require type" << endl;
        for (const auto& type : types) {
            cout << type << " ";
        }
        cout << endl;
        #endif
        for (const auto& type : types) {
            typesArray.append(type);
        }
        Json::Value ret;
        ret["types"] = typesArray;
        ret["result"] = true;
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    }
    else if (info == "item") {
        #ifdef ENABLE_REQUIRE_SAFE_ITEM
        cout << endl;
        cout << "[info]: require item" << endl;
        cout << "idx = " << i << endl;
        #endif
        Item item;
        Json::Value ret;
        if (get_item_information(idx_to_id(i, "item"), item) == 0) {
            ret["name"] = item.name;
            ret["seller_idx"] = id_to_idx(item.seller_id, "user");
            ret["seller_name"] = item.seller_name;
            ret["shop_idx"] = id_to_idx(item.shop_id, "shop");
            ret["shop_name"] = item.shop_name;
            ret["price"] = item.price;
            ret["publishDate"] = item.publishDate;
            ret["publishTime"] = item.publishTime;
            ret["description"] = item.description;
            Json::Value typesArray(Json::arrayValue);
            for (const auto& type : item.types) typesArray.append(type);
            ret["types"] = typesArray;
            ret["result"] = true;
        }
        else {
            ret["result"] = false;
        }
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    }
    else if (info == "shop") {
        #ifdef ENABLE_REQUIRE_SAFE_SHOP
        cout << endl;
        cout << "[info]: require shop" << endl;
        cout << "idx = " << i << endl;
        #endif
        ShopInfo shop;
        Json::Value ret;
        if (get_shop_information(idx_to_id(i, "shop"), shop) == 0) {
            ret["name"] = shop.name;
            ret["manager"] = shop.manager_name;
            ret["manager_idx"] = id_to_idx(shop.manager_id, "user");
            ret["date"] = shop.setupDate;
            ret["state"] = shop.state;
            ret["description"] = shop.description;
            ret["result"] = true;
            string account = req->getHeader("UserAccount");
            string token = req->getHeader("UserToken");
            int user_id = user_get_id(account, token);
            if (user_in_shop(user_id, shop.id)) {
                ret["invite_code"] = shop.invite_code;
            }
        }
        else {
            ret["result"] = false;
        }
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    }
    else if (info == "user") {
        #ifdef ENABLE_REQUIRE_SAFE_USER
        cout << endl;
        cout << "[info]: require user" << endl;
        cout << "idx = " << i << endl;
        #endif
        UserInfo user;
        Json::Value ret;
        if (get_user_information(idx_to_id(i, "user"), user) == 0) {
            ret["name"] = user.name;
            ret["account"] = user.account;
            ret["phone"] = user.phone;
            ret["email"] = user.email;
            ret["description"] = user.description;
            ret["result"] = true;
        }
        else {
            ret["result"] = false;
        }
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
    }
}

void Controller::require_image(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& info, const string& idx) const {
    #ifdef ENABLE_REQUIRE_IMAGE
    cout << endl;
    cout << "[info]: require_image_" << info << endl;
    cout << "idx = " << idx << endl;
    #endif
    std::string image_path = load_image(info, stoi(idx));
    if (image_path.empty()) {
        #ifdef ENABLE_REQUIRE_IMAGE
        cout << "文件在文件系统中不存在" << endl;
        #endif
        auto resp = HttpResponse::newNotFoundResponse();
        callback(resp);
        return;
    }
    #ifdef ENABLE_REQUIRE_IMAGE
    cout << "图片路径: " << image_path << endl;
    #endif
    auto resp = HttpResponse::newFileResponse(image_path);
    // resp->setExpiredTime(3600);
    callback(resp);
}

void Controller::get_shop_idx(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    string account = req->getHeader("UserAccount");
    string token = req->getHeader("UserToken");
    int user_id = user_get_id(account, token);
    Json::Value ret;
    vector<int> shop_ids;
    #ifdef ENABLE_GET_SHOP_IDX
    cout << endl;
    cout << "[info]: getting shop idx for user " << user_id << endl;
    #endif
    if (user_id != -1 && get_user_shop_id(user_id, shop_ids) == 0) {
        ret["user_idx"] = id_to_idx(user_id, "user");
        Json::Value idxArray(Json::arrayValue);
        for (const auto& id : shop_ids) idxArray.append(id_to_idx(id, "shop"));
        ret["shop_idx"] = idxArray;
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
        return;
    }
    ret["user_idx"] = -1;
    ret["shop_idx"] = -1;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::get_item_idx(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& shop_idx) const {
    string account = req->getHeader("UserAccount");
    string token = req->getHeader("UserToken");
    int user_id = user_get_id(account, token);
    #ifdef ENABLE_GET_ITEM_IDX
    cout << endl;
    cout << "[info]: getting item idx for user " << user_id << " in shop " << shop_idx << endl;
    #endif
    Json::Value ret;
    vector<int> item_ids;
    if (user_id != -1 && get_user_shop_item_id(user_id, idx_to_id(stoi(shop_idx), "shop"), item_ids) == 0) {
        ret["user_idx"] = id_to_idx(user_id, "user");
        Json::Value idxArray(Json::arrayValue);
        for (const auto& id : item_ids) idxArray.append(id_to_idx(id, "item"));
        ret["item_idx"] = idxArray;
        auto resp = HttpResponse::newHttpJsonResponse(ret);
        callback(resp);
        return;
    }
    ret["user_idx"] = -1;
    ret["item_idx"] = -1;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::setup_shop(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    string account = req->getHeader("UserAccount");
    string token = req->getHeader("UserToken");
    int manager_id = user_get_id(account, token);
    string shop_name = drogon::utils::urlDecode(req->getHeader("name"));
    string invite_code = req->getHeader("invite_code");
    string description = drogon::utils::urlDecode(req->getHeader("description"));
    #ifdef ENABLE_SETUP_SHOP
    cout << endl;
    cout << "[info]: User " << manager_id << " creating a shop ..." << endl;
    cout << "shop name: " << shop_name << endl;
    cout << "invite code: " << invite_code << endl;
    cout << "description: " << description << endl;
    #endif
    ShopInfo shop;
    shop.name = shop_name;
    shop.invite_code = invite_code;
    shop.description = description;
    shop.manager_id = manager_id;
    Json::Value ret;
    int shop_id = create_shop(manager_id, shop);
    if (shop_id != -1) {
        save_image(req, "shop", id_to_idx(shop_id, "shop"));
    }
    ret["result"] = (shop_id != -1);
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::edit_shop(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& shop_idx) const {
    string clean_shop_idx = shop_idx.substr(0, shop_idx.find_first_of("/"));
    string account = req->getHeader("UserAccount");
    string token = req->getHeader("UserToken");
    int manager_id = user_get_id(account, token);
    string shop_name = drogon::utils::urlDecode(req->getHeader("name"));
    string invite_code = req->getHeader("invite_code");
    string description = drogon::utils::urlDecode(req->getHeader("description"));
    cout << endl;
    cout << "[info]: User " << manager_id << " editing his/her shop " << clean_shop_idx << " ..." << endl;
    cout << "new shop name: " << shop_name << endl;
    cout << "new invite code: " << invite_code << endl;
    cout << "new description: " << description << endl;
    ShopInfo shop;
    shop.name = shop_name;
    shop.invite_code = invite_code;
    shop.description = description;
    cout << "user " << manager_id << " editing his/her shop " << clean_shop_idx << " ..." << endl;
    bool result = (modify_shop_information(manager_id, idx_to_id(stoi(clean_shop_idx), "shop"), shop) == 0);
    if (result) {
        save_image(req, "shop", stoi(clean_shop_idx));
    }
    Json::Value ret;
    ret["result"] = result;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::dismiss_shop(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& shop_idx) const {
    int idx = stoi(shop_idx.substr(0, shop_idx.find_first_of("/")));
    string account = req->getHeader("UserAccount");
    string token = req->getHeader("UserToken");
    int manager_id = user_get_id(account, token);
    cout << endl;
    cout << "[info]: User " << manager_id << " dismissing his/her shop " << idx << " ..." << endl;
    bool result = (delete_shop(manager_id, idx_to_id(idx, "shop")) == 0);
    if (result) {
        delete_image("shop", idx);
    }
    Json::Value ret;
    ret["result"] = result;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::find_shop_by_invite_code(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    auto json = req->getJsonObject();
    string invite_code;
    if(json) {
        invite_code = (*json)["invite_code"].asString();
    }
    int shop_id = get_shop_id(invite_code);
    cout << endl;
    cout << "[info]: finding shop by invite code " << invite_code << endl;
    Json::Value ret;
    ret["shop_idx"] = id_to_idx(shop_id, "shop");
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::join_shop(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    auto json = req->getJsonObject();
    string invite_code;
    if(json) {
        invite_code = (*json)["invite_code"].asString();
    }
    string account = req->getHeader("UserAccount");
    string token = req->getHeader("UserToken");
    int user_id = user_get_id(account, token);
    cout << endl;
    cout << "[info]: User " << user_id << " joining shop with invite code " << invite_code << endl;
    bool result = (attend_shop(user_id, invite_code) == 0);
    Json::Value ret;
    ret["result"] = result;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::quit_shop(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& shop_idx) const {
    cout << "[xxxxxxxxxxxxxxx]" << shop_idx << endl;
    int idx = stoi(shop_idx.substr(0, shop_idx.find_first_of("/")));
    string account = req->getHeader("UserAccount");
    string token = req->getHeader("UserToken");
    int user_id = user_get_id(account, token);
    cout << endl;
    cout << "[info]: User " << user_id << " quitting shop " << idx << endl;
    bool result = (leave_shop(user_id, idx_to_id(idx, "shop")) == 0);
    Json::Value ret;
    ret["result"] = result;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::publish_new_item(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& shop_idx) const {
    int idx_shop = stoi(shop_idx.substr(0, shop_idx.find_first_of("/")));
    string account = req->getHeader("UserAccount");
    string token = req->getHeader("UserToken");
    int seller_id = user_get_id(account, token);
    Item item;
    item.name = drogon::utils::urlDecode(req->getHeader("name"));
    item.price = std::stof(req->getHeader("price"));
    item.description = drogon::utils::urlDecode(req->getHeader("description"));
    for (const pair<const string, const string> &header: req->headers()) {
        auto key = header.first;
        auto value = header.second;
        if (key.rfind("type", 0) == 0) {
            item.types.push_back(drogon::utils::urlDecode(value));
        }
    }
    cout << endl;
    cout << "[info]: user " << seller_id << " publishing item in shop " << shop_idx << ":" << endl;
    cout << "name: " << item.name << endl;
    cout << "price: " << item.name << endl;
    cout << "description: " << item.name << endl;
    cout << "type: ";
    for (const auto type: item.types) cout << type << " ";
    cout << endl;
    int item_id = publish_item(seller_id, idx_to_id(idx_shop, "shop"), item);
    if (item_id != -1) {
        save_image(req, "item", id_to_idx(item_id, "item"));
    }
    Json::Value ret;
    ret["result"] = (item_id != -1);
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::edit_current_item(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& shop_idx, const string& item_idx) const {
    int idx_shop = stoi(shop_idx.substr(0, shop_idx.find_first_of("/")));
    int idx_item = stoi(item_idx.substr(0, item_idx.find_first_of("/")));
    string account = req->getHeader("UserAccount");
    string token = req->getHeader("UserToken");
    int seller_id = user_get_id(account, token);
    Item item;
    item.name = drogon::utils::urlDecode(req->getHeader("name"));
    item.price = std::stof(req->getHeader("price"));
    item.description = drogon::utils::urlDecode(req->getHeader("description"));
    for (const pair<const string, const string> &header: req->headers()) {
        auto key = header.first;
        auto value = header.second;
        if (key.rfind("type", 0) == 0) {
            item.types.push_back(drogon::utils::urlDecode(value));
        }
    }
    cout << endl;
    cout << "[info]: user " << seller_id << " editing item " << item_idx << " in shop " << shop_idx << ":" << endl;
    cout << "name: " << item.name << endl;
    cout << "price: " << item.name << endl;
    cout << "description: " << item.name << endl;
    cout << "type: ";
    for (const auto type: item.types) cout << type << " ";
    cout << endl;
    bool result = (modify_item_information(seller_id, idx_to_id(idx_shop, "shop"), idx_to_id(idx_item, "item"), item) == 0);
    if (result) {
        save_image(req, "item", idx_item);
    }
    Json::Value ret;
    ret["result"] = result;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::item_offshelf(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& shop_idx, const string& item_idx) const {
    int idx_shop = stoi(shop_idx.substr(0, shop_idx.find_first_of("/")));
    int idx_item = stoi(item_idx.substr(0, item_idx.find_first_of("/")));
    string account = req->getHeader("UserAccount");
    string token = req->getHeader("UserToken");
    int seller_id = user_get_id(account, token);
    cout << endl;
    cout << "[info]: user " << seller_id << " taking item " << item_idx << " off shelf in shop " << shop_idx << endl;
    bool result = (delete_item(seller_id, idx_to_id(idx_shop, "shop"), idx_to_id(idx_item, "item")) == 0);
    if (result) {
        delete_image("item", idx_item);
    }
    Json::Value ret;
    ret["result"] = result;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::initialize_order(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& item_idx) const {
    string account = req->getHeader("UserAccount");
    string token = req->getHeader("UserToken");
    int buyer_id = user_get_id(account, token);
    string quantity, address, operation;
    auto json = req->getJsonObject();
    if (json) {
        quantity = (*json)["quantity"].asString();
        address = drogon::utils::urlDecode((*json)["address"].asString());
        operation = (*json)["operation"].asString();
    }
    cout << endl;
    cout << "[info]: user " << buyer_id << "creating order:" << endl;
    cout << "item idx: " << item_idx << endl;
    cout << "quantity: " << quantity << endl;
    cout << "operation: " << operation << endl;
    cout << "address: " << address << endl;
    bool result = (create_order(buyer_id, idx_to_id(stoi(item_idx), "item"), stoi(quantity), address, operation == "buy") == 0);
    Json::Value ret;
    ret["result"] = result;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::acquire_order(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    string account = req->getHeader("UserAccount");
    string token = req->getHeader("UserToken");
    int user_id = user_get_id(account, token);
    auto json = req->getJsonObject();
    bool is_buyer;
    int order_status;
    if (json) {
        is_buyer = (*json)["is_buyer"].asBool();
        order_status = (*json)["order_status"].asInt();
    }
    cout << endl;
    cout << "[info]: user " << user_id << " getting orders with status " << order_status << endl;
    vector<Order> orders;
    Json::Value ret;
    if (get_orders(user_id, is_buyer, order_status, orders) == 0) {
        Json::Value orderIdxArr(Json::arrayValue);
        Json::Value buyerIdxArr(Json::arrayValue);
        Json::Value buyerNameArr(Json::arrayValue);
        Json::Value sellerIdxArr(Json::arrayValue);
        Json::Value sellerNameArr(Json::arrayValue);
        Json::Value shopIdxArr(Json::arrayValue);
        Json::Value shopNameArr(Json::arrayValue);
        Json::Value itemIdxArr(Json::arrayValue);
        Json::Value itemNameArr(Json::arrayValue);
        Json::Value quantityArr(Json::arrayValue);
        Json::Value totalPriceArr(Json::arrayValue);
        Json::Value addressArr(Json::arrayValue);
        Json::Value dateArr(Json::arrayValue);
        Json::Value timeArr(Json::arrayValue);
        Json::Value statusArr(Json::arrayValue);
        for (const auto& order: orders) {
            orderIdxArr.append(id_to_idx(order.id, "order"));
            buyerIdxArr.append(id_to_idx(order.buyer_id, "user"));
            buyerNameArr.append(order.buyer_name);
            sellerIdxArr.append(id_to_idx(order.seller_id, "user"));
            sellerNameArr.append(order.seller_name);
            shopIdxArr.append(id_to_idx(order.shop_id, "shop"));
            shopNameArr.append(order.shop_name);
            itemIdxArr.append(id_to_idx(order.item_id, "item"));
            itemNameArr.append(order.item_name);
            quantityArr.append(order.quantity);
            totalPriceArr.append(order.total_price);
            addressArr.append(order.address);
            dateArr.append(order.createDate);
            timeArr.append(order.createTime);
            statusArr.append(order.state);
        }
        ret["order_idx"] = orderIdxArr;
        ret["buyer_idx"] = buyerIdxArr;
        ret["buyer_name"] = buyerNameArr;
        ret["seller_idx"] = sellerIdxArr;
        ret["seller_name"] = sellerNameArr;
        ret["shop_idx"] = shopIdxArr;
        ret["shop_name"] = shopNameArr;
        ret["item_idx"] = itemIdxArr;
        ret["item_name"] = itemNameArr;
        ret["quantity"] = quantityArr;
        ret["total_price"] = totalPriceArr;
        ret["address"] = addressArr;
        ret["order_date"] = dateArr;
        ret["order_time"] = timeArr;
        ret["order_status"] = statusArr;
    }
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::modify_order(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback, const string& order_idx) const {
    string account = req->getHeader("UserAccount");
    string token = req->getHeader("UserToken");
    int user_id = user_get_id(account, token);
    auto json = req->getJsonObject();
    string op;
    if (json) {
        op = (*json)["operation"].asString();
    }
    cout << endl;
    cout << "[info]: user " << user_id << " editing order " << order_idx << " with operation " << op << endl;
    bool is_buyer = (op == "pay")
                 || (op == "buyer")
                 || (op == "confirm")
                 || (op == "return");
    bool result = (edit_order_state(user_id, idx_to_id(stoi(order_idx), "order"), is_buyer, op) == 0);
    Json::Value ret;
    ret["result"] = result;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::user_edit_info(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    string account = req->getHeader("UserAccount");
    string token = req->getHeader("UserToken");
    UserInfo user;
    user.name = drogon::utils::urlDecode(req->getHeader("name"));
    user.phone = req->getHeader("phone_number");
    user.email = req->getHeader("email");
    user.description = drogon::utils::urlDecode(req->getHeader("description"));
    bool result = (user_modify_information(account, token, user) == 0);
    if (result) {
        int user_id = user_get_id(account, token);
        save_image(req, "user", id_to_idx(user_id, "user"));
    }
    Json::Value ret;
    ret["result"] = result;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::user_edit_account(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    string account = req->getHeader("UserAccount");
    string token = req->getHeader("UserToken");
    string password, new_account;
    auto json = req->getJsonObject();
    if (json) {
        password = (*json)["password"].asString();
        new_account = (*json)["new_account"].asString();
    }
    bool result = (user_modify_account(account, token, password, new_account) == 0);
    Json::Value ret;
    ret["result"] = result;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::user_edit_password(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    string account = req->getHeader("UserAccount");
    string token = req->getHeader("UserToken");
    string old_password, new_password;
    auto json = req->getJsonObject();
    if (json) {
        old_password = (*json)["old_password"].asString();
        new_password = (*json)["new_password"].asString();
    }
    bool result = (user_modify_password(account, token, old_password, new_password) == 0);
    Json::Value ret;
    ret["result"] = result;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::logout(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    string account = req->getHeader("UserAccount");
    string token = req->getHeader("UserToken");
    Json::Value ret;
    ret["result"] = (user_logout(account, token) == 0);
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::delete_account(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    string account = req->getHeader("UserAccount");
    string token = req->getHeader("UserToken");
    int user_id = user_get_id(account, token);
    bool result = (user_delete_account(account, token) == 0);
    if (result) {
        delete_image("user", id_to_idx(user_id, "user"));
    }
    Json::Value ret;
    ret["result"] = result;
    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void Controller::get_history_liaison(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    int my_id = user_get_id(req->getHeader("UserAccount"), req->getHeader("UserToken"));
    if (my_id == -1) { return; }

    std::vector<ContactStat> stats;
    get_chat_contacts(my_id, stats); // 调用上面的 SQL

    Json::Value ret;
    Json::Value arr(Json::arrayValue);

    for (const auto& stat : stats) {
        UserInfo u;
        get_user_information(stat.user_id, u); // 获取头像名字
        
        Json::Value item;
        item["user_idx"] = stat.user_id;
        item["name"] = u.name;
        item["unread"] = stat.unread_count;      // 新增：未读数
        item["last_time"] = stat.last_datetime;  // 新增：用于前端二次确认或显示
        // item["preview"] = stat.last_content;  // 可选：消息预览
        arr.append(item);
    }
    ret["liaisons"] = arr;
    ret["result"] = true;
    callback(HttpResponse::newHttpJsonResponse(ret));
}

void Controller::send_message(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    int my_id = user_get_id(req->getHeader("UserAccount"), req->getHeader("UserToken"));
    auto json = req->getJsonObject();
    int target_idx = (*json)["target_idx"].asInt();
    std::string content = (*json)["content"].asString();

    if (my_id == idx_to_id(target_idx, "user")) {
        Json::Value ret;
        ret["result"] = false;
        callback(HttpResponse::newHttpJsonResponse(ret));
        return;
    }
    
    // 获取当前时间
    auto now = trantor::Date::now();
    std::string date = now.toCustomFormattedString("%Y-%m-%d");
    std::string time = now.toCustomFormattedString("%H:%M:%S");

    #ifdef ENABLE_SEND_MESSAGE
    cout << endl;
    cout << "[info]: User " << my_id << " sending message to user " << target_idx << ":" << endl;
    cout << "content: " << content << endl;
    cout << "date: " << date << ", time: " << time << endl;
    #endif

    bool success = (save_message(my_id, target_idx, content, date, time) == 0);
    
    Json::Value ret;
    ret["result"] = success;
    callback(HttpResponse::newHttpJsonResponse(ret));
}

void Controller::receive_latest(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    int my_id = user_get_id(req->getHeader("UserAccount"), req->getHeader("UserToken"));
    auto json = req->getJsonObject();
    int target_idx = (*json)["target_idx"].asInt();
    std::string last_date = (*json)["last_date"].asString();
    std::string last_time = (*json)["last_time"].asString();

    std::vector<Message> msgs;
    // 查询对方发给我的新消息
    get_latest_messages(target_idx, my_id, last_date, last_time, msgs);
    #ifdef ENABLE_RECEIVE_LATEST
    if (msgs.size() > 0) {
        cout << endl;
        cout << "[info]: User " << my_id << " fetching new messages from user " << target_idx << " after " << last_date << " " << last_time << endl;
        cout << "fetched " << msgs.size() << " messages." << endl;
    }
    #endif

    Json::Value ret;
    Json::Value arr(Json::arrayValue);
    for (int i = msgs.size() - 1; i >= 0; --i) {
        const auto& m = msgs[i];
        Json::Value item;
        item["content"] = m.content;
        item["date"] = m.sendDate;
        item["time"] = m.sendTime;
        item["sender_idx"] = m.sender_id;
        arr.append(item);
    }
    ret["messages"] = arr;
    ret["result"] = true;
    callback(HttpResponse::newHttpJsonResponse(ret));
}

void Controller::receive_history(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    int my_id = user_get_id(req->getHeader("UserAccount"), req->getHeader("UserToken"));
    auto json = req->getJsonObject();
    int target_idx = (*json)["target_idx"].asInt();
    
    // 获取可选的分页参数
    std::string before_date;
    std::string before_time;
    
    if (json->isMember("before_date") && json->isMember("before_time")) {
        before_date = (*json)["before_date"].asString();
        before_time = (*json)["before_time"].asString();
    } else {
        // 如果没传，就给一个极大值（例如当前时间，或者 "9999-12-31"），表示获取最新的历史
        auto now = trantor::Date::now();
        before_date = now.toCustomFormattedString("%Y-%m-%d");
        before_time = now.toCustomFormattedString("%H:%M:%S");
    }

    std::vector<Message> msgs;
    // 调用新的分页查询函数，一次查 20 条
    get_history_messages_paged(my_id, target_idx, before_date, before_time, 20, msgs);

    #ifdef ENABLE_RECEIVE_HISTORY
    if (msgs.size() > 0) {
        cout << endl;
        cout << "[info]: User " << my_id << " fetching history messages with user " << target_idx 
            << " before " << before_date << " " << before_time << endl;
        cout << "fetched " << msgs.size() << " messages." << endl;
    }
    #endif
    
    // 数据库查出来是倒序的 (Time: 10:00, 09:59, 09:58...)
    // 我们需要反转成正序发给前端 (Time: 09:58, 09:59, 10:00...)
    // std::reverse(msgs.begin(), msgs.end());

    Json::Value ret;
    Json::Value arr(Json::arrayValue);
    for (const auto& m : msgs) {
        Json::Value item;
        item["content"] = m.content;
        item["date"] = m.sendDate;
        item["time"] = m.sendTime;
        item["sender_idx"] = m.sender_id;
        // 建议加上 id，虽然前端暂时没用到，但作为唯一标识很主要
        item["id"] = m.id; 
        arr.append(item);
    }
    ret["messages"] = arr;
    ret["result"] = true;
    callback(HttpResponse::newHttpJsonResponse(ret));
}

void Controller::mark_read(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) const {
    int my_id = user_get_id(req->getHeader("UserAccount"), req->getHeader("UserToken"));
    auto json = req->getJsonObject();
    int target_idx = (*json)["target_idx"].asInt();

    #ifdef ENABLE_MARK_READ
    cout << endl;
    cout << "[info]: User " << my_id << " marking messages as read from user " << target_idx << endl;
    #endif

    // 调用数据库更新: UPDATE messages SET is_read=1 WHERE sender=target AND receiver=me
    mark_messages_as_read(my_id, target_idx);

    Json::Value ret;
    ret["result"] = true;
    callback(HttpResponse::newHttpJsonResponse(ret));
}