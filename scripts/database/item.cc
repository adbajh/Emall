#include "database.h"
#include <sstream> // for building SQL string

// (4) 商品相关

/**
 * @brief 发布新商品
 * @param user_id 发布者id
 * @param shop_id 商店id
 * @param item 包含商品信息的结构体 (name, price, description, types)
 * @return 成功则返回新创建的商品ID, 失败返回-1
 */
int publish_item(int user_id, int shop_id, const Item& item) {
    sqlite3* db;
    // 失败路径1: 数据库打开失败
    if (sqlite3_open(database_name, &db) != SQLITE_OK) return -1;

    sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);

    // 1. 检查 user_id 是否有权限在此 shop_id 发布商品 (是经理或销售员)
    bool authorized = false;
    const char* sql_auth = "SELECT 1 FROM shops WHERE id = ? AND manager_id = ? "
                           "UNION ALL "
                           "SELECT 1 FROM seller_shops WHERE shop_id = ? AND seller_id = ?;";
    sqlite3_stmt* stmt_auth;
    if (sqlite3_prepare_v2(db, sql_auth, -1, &stmt_auth, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_auth, 1, shop_id);
        sqlite3_bind_int(stmt_auth, 2, user_id);
        sqlite3_bind_int(stmt_auth, 3, shop_id);
        sqlite3_bind_int(stmt_auth, 4, user_id);
        if (sqlite3_step(stmt_auth) == SQLITE_ROW) {
            authorized = true;
        }
        sqlite3_finalize(stmt_auth);
    }
    if (!authorized) {
        // 失败路径2: 无权限
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return -1;
    }
    
    // 2. 插入商品基本信息
    long long item_id = -1; // 初始化为失败状态
    const char* sql_insert_item = "INSERT INTO items (name, seller_id, shop_id, price, publish_date, publish_time, description) VALUES (?, ?, ?, ?, date('now'), time('now'), ?);";
    sqlite3_stmt* stmt_insert_item;
    if (sqlite3_prepare_v2(db, sql_insert_item, -1, &stmt_insert_item, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt_insert_item, 1, item.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt_insert_item, 2, user_id);
        sqlite3_bind_int(stmt_insert_item, 3, shop_id);
        sqlite3_bind_double(stmt_insert_item, 4, item.price);
        sqlite3_bind_text(stmt_insert_item, 5, item.description.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt_insert_item) == SQLITE_DONE) {
            // *** 核心改动：获取新插入行的ID ***
            item_id = sqlite3_last_insert_rowid(db);
        }
        sqlite3_finalize(stmt_insert_item);
    }
    if (item_id == -1) {
        // 失败路径3: 插入商品基本信息失败
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return -1;
    }

    // 3. 处理商品类别
    bool category_ok = true;
    for (const auto& type_name : item.types) {
        // 查找或创建类别
        long long category_id = -1;
        const char* sql_find_cat = "SELECT id FROM categories WHERE name = ?;";
        sqlite3_stmt* stmt_find_cat;
        if (sqlite3_prepare_v2(db, sql_find_cat, -1, &stmt_find_cat, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt_find_cat, 1, type_name.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt_find_cat) == SQLITE_ROW) {
                category_id = sqlite3_column_int64(stmt_find_cat, 0);
            }
            sqlite3_finalize(stmt_find_cat);
        }

        if (category_id == -1) { // 类别不存在，创建它
            const char* sql_insert_cat = "INSERT INTO categories (name) VALUES (?);";
            sqlite3_stmt* stmt_insert_cat;
            if (sqlite3_prepare_v2(db, sql_insert_cat, -1, &stmt_insert_cat, 0) == SQLITE_OK) {
                sqlite3_bind_text(stmt_insert_cat, 1, type_name.c_str(), -1, SQLITE_TRANSIENT);
                if (sqlite3_step(stmt_insert_cat) == SQLITE_DONE) {
                    category_id = sqlite3_last_insert_rowid(db);
                }
                sqlite3_finalize(stmt_insert_cat);
            }
        }
        if (category_id == -1) { category_ok = false; break; }

        // 关联商品和类别
        const char* sql_link = "INSERT INTO item_categories (item_id, category_id) VALUES (?, ?);";
        sqlite3_stmt* stmt_link;
        if (sqlite3_prepare_v2(db, sql_link, -1, &stmt_link, 0) == SQLITE_OK) {
            sqlite3_bind_int64(stmt_link, 1, item_id);
            sqlite3_bind_int64(stmt_link, 2, category_id);
            if (sqlite3_step(stmt_link) != SQLITE_DONE) { category_ok = false; }
            sqlite3_finalize(stmt_link);
        } else { category_ok = false; }
        if (!category_ok) break;
    }

    // 4. 根据处理结果提交或回滚事务，并返回相应的值
    int return_value;
    if (category_ok) {
        sqlite3_exec(db, "COMMIT;", 0, 0, 0);
        return_value = item_id; // 成功，返回新ID
    } else {
        // 失败路径4: 处理类别时发生错误
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        return_value = -1; // 失败，返回-1
    }
    
    sqlite3_close(db);
    return return_value;
}

/**
 * @brief 修改商品信息
 * @param user_id 操作者id
 * @param shop_id 商店id
 * @param item_id 商品id
 * @param item 包含新信息的结构体
 * @return 0成功, 1失败 (无权限, 数据库错误)
 */
int modify_item_information(int user_id, int shop_id, int item_id, const Item& item) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) return 1;

    sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);
    
    // 1. 验证权限
    bool authorized = false;
    const char* sql_auth = "SELECT 1 FROM items WHERE id = ? AND seller_id = ? AND shop_id = ?;";
    sqlite3_stmt* stmt_auth;
    if (sqlite3_prepare_v2(db, sql_auth, -1, &stmt_auth, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_auth, 1, item_id);
        sqlite3_bind_int(stmt_auth, 2, user_id);
        sqlite3_bind_int(stmt_auth, 3, shop_id);
        if (sqlite3_step(stmt_auth) == SQLITE_ROW) {
            authorized = true;
        }
        sqlite3_finalize(stmt_auth);
    }
    if (!authorized) {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return 1;
    }

    // 2. 更新商品基本信息
    bool update_ok = false;
    const char* sql_update_item = "UPDATE items SET name = ?, price = ?, description = ? WHERE id = ?;";
    sqlite3_stmt* stmt_update_item;
    if (sqlite3_prepare_v2(db, sql_update_item, -1, &stmt_update_item, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt_update_item, 1, item.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt_update_item, 2, item.price);
        sqlite3_bind_text(stmt_update_item, 3, item.description.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt_update_item, 4, item_id);
        if (sqlite3_step(stmt_update_item) == SQLITE_DONE) {
            update_ok = true;
        }
        sqlite3_finalize(stmt_update_item);
    }
    if (!update_ok) {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return 1;
    }
    
    // 3. 更新类别 (先删除旧的，再添加新的)
    bool category_ok = true;
    const char* sql_delete_cats = "DELETE FROM item_categories WHERE item_id = ?;";
    sqlite3_stmt* stmt_delete_cats;
    if (sqlite3_prepare_v2(db, sql_delete_cats, -1, &stmt_delete_cats, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_delete_cats, 1, item_id);
        if (sqlite3_step(stmt_delete_cats) != SQLITE_DONE) category_ok = false;
        sqlite3_finalize(stmt_delete_cats);
    } else category_ok = false;

    if (category_ok) { // 如果删除成功，则添加新的
        // (此段逻辑与 publish_item 中完全相同)
        for (const auto& type_name : item.types) {
            long long category_id = -1;
            const char* sql_find_cat = "SELECT id FROM categories WHERE name = ?;";
            sqlite3_stmt* stmt_find_cat;
            if (sqlite3_prepare_v2(db, sql_find_cat, -1, &stmt_find_cat, 0) == SQLITE_OK) {
                sqlite3_bind_text(stmt_find_cat, 1, type_name.c_str(), -1, SQLITE_STATIC);
                if (sqlite3_step(stmt_find_cat) == SQLITE_ROW) category_id = sqlite3_column_int64(stmt_find_cat, 0);
                sqlite3_finalize(stmt_find_cat);
            }
            if (category_id == -1) {
                const char* sql_insert_cat = "INSERT INTO categories (name) VALUES (?);";
                sqlite3_stmt* stmt_insert_cat;
                if (sqlite3_prepare_v2(db, sql_insert_cat, -1, &stmt_insert_cat, 0) == SQLITE_OK) {
                    sqlite3_bind_text(stmt_insert_cat, 1, type_name.c_str(), -1, SQLITE_TRANSIENT);
                    if (sqlite3_step(stmt_insert_cat) == SQLITE_DONE) category_id = sqlite3_last_insert_rowid(db);
                    sqlite3_finalize(stmt_insert_cat);
                }
            }
            if (category_id == -1) { category_ok = false; break; }
            const char* sql_link = "INSERT INTO item_categories (item_id, category_id) VALUES (?, ?);";
            sqlite3_stmt* stmt_link;
            if (sqlite3_prepare_v2(db, sql_link, -1, &stmt_link, 0) == SQLITE_OK) {
                sqlite3_bind_int(stmt_link, 1, item_id);
                sqlite3_bind_int64(stmt_link, 2, category_id);
                if (sqlite3_step(stmt_link) != SQLITE_DONE) category_ok = false;
                sqlite3_finalize(stmt_link);
            } else category_ok = false;
            if (!category_ok) break;
        }
    }

    if (category_ok) {
        sqlite3_exec(db, "COMMIT;", 0, 0, 0);
    } else {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
    }
    sqlite3_close(db);
    return category_ok ? 0 : 1;
}

/**
 * @brief 删除商品
 * @param user_id 操作者id
 * @param shop_id 商店id
 * @param item_id 商品id
 * @return 0成功, 1失败 (无权限, 有进行中的订单, 数据库错误)
 */
int delete_item(int user_id, int shop_id, int item_id) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) return 1;
    sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);

    // 1. 验证权限
    bool authorized = false;
    const char* sql_auth = "SELECT 1 FROM items WHERE id = ? AND seller_id = ? AND shop_id = ?;";
    sqlite3_stmt* stmt_auth;
    if (sqlite3_prepare_v2(db, sql_auth, -1, &stmt_auth, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_auth, 1, item_id);
        sqlite3_bind_int(stmt_auth, 2, user_id);
        sqlite3_bind_int(stmt_auth, 3, shop_id);
        if (sqlite3_step(stmt_auth) == SQLITE_ROW) authorized = true;
        sqlite3_finalize(stmt_auth);
    }
    if (!authorized) {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return 1;
    }

    // 2. 检查进行中的订单
    bool has_ongoing_orders = false;
    const char* sql_check_orders = "SELECT 1 FROM orders WHERE item_id = ? AND order_status NOT IN (3, 5) LIMIT 1;";
    sqlite3_stmt* stmt_check_orders;
    if (sqlite3_prepare_v2(db, sql_check_orders, -1, &stmt_check_orders, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_check_orders, 1, item_id);
        if (sqlite3_step(stmt_check_orders) == SQLITE_ROW) has_ongoing_orders = true;
        sqlite3_finalize(stmt_check_orders);
    }
    if (has_ongoing_orders) {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return 1;
    }
    
    // 3. 删除商品 (item_categories 会被级联删除)
    int result = 1;
    const char* sql_delete = "DELETE FROM items WHERE id = ?;";
    sqlite3_stmt* stmt_delete;
    if (sqlite3_prepare_v2(db, sql_delete, -1, &stmt_delete, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_delete, 1, item_id);
        if (sqlite3_step(stmt_delete) == SQLITE_DONE && sqlite3_changes(db) > 0) result = 0;
        sqlite3_finalize(stmt_delete);
    }

    if (result == 0) sqlite3_exec(db, "COMMIT;", 0, 0, 0);
    else sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
    sqlite3_close(db);
    return result;
}

/**
 * @brief 获取用户在某商店发布的所有商品id
 * @param user_id 用户id
 * @param shop_id 商店id
 * @param id_vec [out] 存放商品id的vector
 * @return 0成功, 1失败 (无权限, 数据库错误)
 */
int get_user_shop_item_id(int user_id, int shop_id, vector<int>& id_vec) {
    id_vec.clear();
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) return 1;

    // 1. 验证用户是否是该店成员
    bool authorized = false;
    const char* sql_auth = "SELECT 1 FROM shops WHERE id = ? AND manager_id = ? "
                           "UNION ALL "
                           "SELECT 1 FROM seller_shops WHERE shop_id = ? AND seller_id = ?;";
    sqlite3_stmt* stmt_auth;
    if (sqlite3_prepare_v2(db, sql_auth, -1, &stmt_auth, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_auth, 1, shop_id);
        sqlite3_bind_int(stmt_auth, 2, user_id);
        sqlite3_bind_int(stmt_auth, 3, shop_id);
        sqlite3_bind_int(stmt_auth, 4, user_id);
        if (sqlite3_step(stmt_auth) == SQLITE_ROW) authorized = true;
        sqlite3_finalize(stmt_auth);
    }
    if (!authorized) {
        sqlite3_close(db);
        return 1; // 严格来说这里应该返回1，表示失败
    }
    
    // 2. 获取商品ID
    const char* sql_get = "SELECT id FROM items WHERE seller_id = ? AND shop_id = ? ORDER BY id;";
    sqlite3_stmt* stmt_get;
    if (sqlite3_prepare_v2(db, sql_get, -1, &stmt_get, 0) != SQLITE_OK) {
        sqlite3_close(db);
        return 1;
    }
    sqlite3_bind_int(stmt_get, 1, user_id);
    sqlite3_bind_int(stmt_get, 2, shop_id);
    while (sqlite3_step(stmt_get) == SQLITE_ROW) {
        id_vec.push_back(sqlite3_column_int(stmt_get, 0));
    }
    sqlite3_finalize(stmt_get);
    sqlite3_close(db);
    return 0;
}

/**
 * @brief 搜索满足条件的下一个商品id
 * @param cur_id 当前商品id, 结果必须大于此id
 * @param cond 搜索条件map
 * @return 找到则返回商品id, 否则返回-1
 */
int search_next_item_id(int cur_id, map<string, string> cond) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) return -1;

    std::stringstream sql;
    vector<string> params;
    sql << "SELECT DISTINCT i.id FROM items i ";

    // -- Joins --
    bool has_type_cond = false;
    for(auto const& [key, val] : cond) if(key.rfind("type", 0) == 0) has_type_cond = true;
    if (has_type_cond) sql << "JOIN item_categories ic ON i.id = ic.item_id JOIN categories c ON ic.category_id = c.id ";
    if (cond.count("shop")) sql << "JOIN shops s ON i.shop_id = s.id ";
    if (cond.count("seller")) sql << "JOIN users u ON i.seller_id = u.id ";
    
    sql << "WHERE i.id > ? ";
    params.push_back(std::to_string(cur_id));

    // -- Conditions --
    vector<string> keywords, types;
    for(auto const& [key, val] : cond) {
        if(key.rfind("key", 0) == 0) keywords.push_back(val);
        else if(key.rfind("type", 0) == 0) types.push_back(val);
    }
    if (!keywords.empty()) {
        sql << "AND (";
        for (size_t i = 0; i < keywords.size(); ++i) {
            sql << (i > 0 ? "OR " : "") << "(i.name LIKE ? OR i.description LIKE ?) ";
            params.push_back("%" + keywords[i] + "%");
            params.push_back("%" + keywords[i] + "%");
        }
        sql << ") ";
    }
    if (!types.empty()) {
        sql << "AND c.name IN (";
        for (size_t i = 0; i < types.size(); ++i) {
            sql << (i > 0 ? ",?" : "?");
            params.push_back(types[i]);
        }
        sql << ") ";
    }
    if (cond.count("shop_idx")) {
        sql << "AND i.shop_id = ? "; params.push_back(cond["shop_idx"]);
    } else if (cond.count("shop")) {
        sql << "AND s.name = ? "; params.push_back(cond["shop"]);
    }
    if (cond.count("seller_idx")) {
        sql << "AND i.seller_id = ? "; params.push_back(cond["seller_idx"]);
    } else if (cond.count("seller")) {
        sql << "AND u.name = ? "; params.push_back(cond["seller"]);
    }
    if (cond.count("min_price")) { sql << "AND i.price >= ? "; params.push_back(cond["min_price"]); }
    if (cond.count("max_price")) { sql << "AND i.price <= ? "; params.push_back(cond["max_price"]); }
    if (cond.count("min_date")) { sql << "AND i.publish_date >= ? "; params.push_back(cond["min_date"]); }
    if (cond.count("max_date")) { sql << "AND i.publish_date <= ? "; params.push_back(cond["max_date"]); }

    sql << "ORDER BY i.id ASC LIMIT 1;";

    // -- Execution --
    int next_id = -1;
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, 0) == SQLITE_OK) {
        for (size_t i = 0; i < params.size(); ++i) {
            sqlite3_bind_text(stmt, i + 1, params[i].c_str(), -1, SQLITE_TRANSIENT);
        }
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            next_id = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return next_id;
}

/**
 * @brief 根据id获取商品所有信息
 * @param id 商品id
 * @param item [out] 存放商品信息的结构体
 * @return 0成功, 1失败 (id不存在)
 */
int get_item_information(int id, Item& item) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) return 1;

    // 1. 获取基本信息
    bool found = false;
    const char* sql_item = "SELECT i.id, i.name, i.seller_id, u.name, i.shop_id, s.name, i.price, i.publish_date, i.publish_time, i.description "
                         "FROM items i JOIN users u ON i.seller_id = u.id JOIN shops s ON i.shop_id = s.id WHERE i.id = ?;";
    sqlite3_stmt* stmt_item;
    if (sqlite3_prepare_v2(db, sql_item, -1, &stmt_item, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_item, 1, id);
        if (sqlite3_step(stmt_item) == SQLITE_ROW) {
            found = true;
            auto get_text = [&](int col_idx) {
                const char* text = (const char*)sqlite3_column_text(stmt_item, col_idx);
                return text ? std::string(text) : "";
            };
            item.id = sqlite3_column_int(stmt_item, 0);
            item.name = get_text(1);
            item.seller_id = sqlite3_column_int(stmt_item, 2);
            item.seller_name = get_text(3);
            item.shop_id = sqlite3_column_int(stmt_item, 4);
            item.shop_name = get_text(5);
            item.price = sqlite3_column_double(stmt_item, 6);
            item.publishDate = get_text(7);
            item.publishTime = get_text(8);
            item.description = get_text(9);
            item.types.clear();
        }
        sqlite3_finalize(stmt_item);
    }
    if (!found) {
        sqlite3_close(db);
        return 1;
    }
    
    // 2. 获取类别信息
    const char* sql_cats = "SELECT c.name FROM categories c JOIN item_categories ic ON c.id = ic.category_id WHERE ic.item_id = ?;";
    sqlite3_stmt* stmt_cats;
    if (sqlite3_prepare_v2(db, sql_cats, -1, &stmt_cats, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_cats, 1, id);
        while (sqlite3_step(stmt_cats) == SQLITE_ROW) {
            item.types.push_back((const char*)sqlite3_column_text(stmt_cats, 0));
        }
        sqlite3_finalize(stmt_cats);
    }
    
    sqlite3_close(db);
    return 0;
}