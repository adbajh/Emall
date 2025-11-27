#include "database.h"

/**
 * @brief 创建店铺
 * @param user_id 创建者(经理)的用户id
 * @param shop 包含店铺信息的结构体 (name, invite_code, description)
 * @return 成功则返回新创建的商店ID, 失败返回-1
 */
int create_shop(int user_id, const ShopInfo& shop) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        return -1;
    }

    sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);

    // 1. 检查 user_id 是否存在
    bool user_exists = false;
    const char* sql_check_user = "SELECT 1 FROM users WHERE id = ?;";
    sqlite3_stmt* stmt_check_user;
    if (sqlite3_prepare_v2(db, sql_check_user, -1, &stmt_check_user, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_check_user, 1, user_id);
        if (sqlite3_step(stmt_check_user) == SQLITE_ROW) {
            user_exists = true;
        }
        sqlite3_finalize(stmt_check_user);
    }
    if (!user_exists) {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return -1; // 用户不存在
    }

    // 2. 插入新店铺
    int new_shop_id = -1; // 初始化为失败状态
    const char* sql_insert = "INSERT INTO shops (name, invite_code, manager_id, creation_date, status, description) VALUES (?, ?, ?, date('now'), 1, ?);";
    sqlite3_stmt* stmt_insert;
    if (sqlite3_prepare_v2(db, sql_insert, -1, &stmt_insert, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt_insert, 1, shop.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt_insert, 2, shop.invite_code.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt_insert, 3, user_id);
        sqlite3_bind_text(stmt_insert, 4, shop.description.c_str(), -1, SQLITE_TRANSIENT);

        // 如果邀请码重复 (UNIQUE constraint)，step会失败
        if (sqlite3_step(stmt_insert) == SQLITE_DONE) {
            // 插入成功，获取新行的ID
            new_shop_id = sqlite3_last_insert_rowid(db);
        }
        sqlite3_finalize(stmt_insert);
    }

    // 3. 根据插入结果提交或回滚事务
    if (new_shop_id != -1) {
        sqlite3_exec(db, "COMMIT;", 0, 0, 0);
    } else {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
    }

    sqlite3_close(db);

    // 4. 返回新ID或-1
    return new_shop_id;
}

/**
 * @brief 修改店铺信息
 * @param user_id 操作者用户id (必须是店铺经理)
 * @param shop_id 要修改的店铺id
 * @param shop 包含新信息的结构体 (name, invite_code, description, state)
 * @return 0成功, 1失败 (非店铺经理操作, 邀请码重复, 数据库错误)
 */
int modify_shop_information(int user_id, int shop_id, const ShopInfo& shop) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        return 1;
    }
    sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);

    // 1. 验证操作者是否为店铺经理
    bool is_manager = false;
    const char* sql_check_manager = "SELECT 1 FROM shops WHERE id = ? AND manager_id = ?;";
    sqlite3_stmt* stmt_check_manager;
    if (sqlite3_prepare_v2(db, sql_check_manager, -1, &stmt_check_manager, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_check_manager, 1, shop_id);
        sqlite3_bind_int(stmt_check_manager, 2, user_id);
        if (sqlite3_step(stmt_check_manager) == SQLITE_ROW) {
            is_manager = true;
        }
        sqlite3_finalize(stmt_check_manager);
    }
    if (!is_manager) {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return 1; // 权限不足
    }
    
    // 2. 执行更新
    int result = 1;
    // 将 string state 转换为 SMALLINT status
    int status = (shop.state == "营业中") ? 1 : 0;
    const char* sql_update = "UPDATE shops SET name = ?, invite_code = ?, description = ?, status = ? WHERE id = ?;";
    sqlite3_stmt* stmt_update;
    if (sqlite3_prepare_v2(db, sql_update, -1, &stmt_update, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt_update, 1, shop.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt_update, 2, shop.invite_code.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt_update, 3, shop.description.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt_update, 4, status);
        sqlite3_bind_int(stmt_update, 5, shop_id);
        
        // 如果邀请码重复，会返回 SQLITE_CONSTRAINT
        if (sqlite3_step(stmt_update) == SQLITE_DONE) {
            if (sqlite3_changes(db) > 0) {
                 result = 0;
            }
        }
        sqlite3_finalize(stmt_update);
    }
    
    if (result == 0) {
        sqlite3_exec(db, "COMMIT;", 0, 0, 0);
    } else {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
    }
    sqlite3_close(db);
    return result;
}

/**
 * @brief 删除店铺
 * @param user_id 操作者用户id (必须是店铺经理)
 * @param shop_id 要删除的店铺id
 * @return 0成功, 1失败 (非店铺经理操作, 店铺有进行中的订单, 数据库错误)
 */
int delete_shop(int user_id, int shop_id) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        return 1;
    }
    sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);

    // 1. 验证操作者是否为店铺经理
    bool is_manager = false;
    const char* sql_check_manager = "SELECT 1 FROM shops WHERE id = ? AND manager_id = ?;";
    sqlite3_stmt* stmt_check_manager;
    if (sqlite3_prepare_v2(db, sql_check_manager, -1, &stmt_check_manager, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_check_manager, 1, shop_id);
        sqlite3_bind_int(stmt_check_manager, 2, user_id);
        if (sqlite3_step(stmt_check_manager) == SQLITE_ROW) {
            is_manager = true;
        }
        sqlite3_finalize(stmt_check_manager);
    }
    if (!is_manager) {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return 1; // 权限不足
    }

    // 2. 检查店铺是否有进行中的订单 (状态不为 3-完成 或 5-无效)
    bool has_ongoing_orders = false;
    const char* sql_check_orders = "SELECT 1 FROM orders WHERE shop_id = ? AND order_status NOT IN (3, 5) LIMIT 1;";
    sqlite3_stmt* stmt_check_orders;
    if (sqlite3_prepare_v2(db, sql_check_orders, -1, &stmt_check_orders, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_check_orders, 1, shop_id);
        if (sqlite3_step(stmt_check_orders) == SQLITE_ROW) {
            has_ongoing_orders = true;
        }
        sqlite3_finalize(stmt_check_orders);
    }
    if (has_ongoing_orders) {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return 1; // 存在进行中的订单
    }

    // 3. 删除店铺 (相关 item, seller_shops 会被级联删除)
    int result = 1;
    const char* sql_delete = "DELETE FROM shops WHERE id = ?;";
    sqlite3_stmt* stmt_delete;
    if (sqlite3_prepare_v2(db, sql_delete, -1, &stmt_delete, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_delete, 1, shop_id);
        if (sqlite3_step(stmt_delete) == SQLITE_DONE && sqlite3_changes(db) > 0) {
            result = 0;
        }
        sqlite3_finalize(stmt_delete);
    }

    if (result == 0) {
        sqlite3_exec(db, "COMMIT;", 0, 0, 0);
    } else {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
    }
    sqlite3_close(db);
    return result;
}

/**
 * @brief 通过邀请码查找商店id
 * @param invite_code 商店邀请码
 * @return 成功返回商店id, 失败返回-1
 */
int get_shop_id(const string& invite_code) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        return -1;
    }
    const char* sql = "SELECT id FROM shops WHERE invite_code = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        sqlite3_close(db);
        return -1;
    }
    sqlite3_bind_text(stmt, 1, invite_code.c_str(), -1, SQLITE_STATIC);
    
    int shop_id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        shop_id = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return shop_id;
}

/**
 * @brief 用户加入店铺
 * @param user_id 要加入的用户id
 * @param invite_code 店铺邀请码
 * @return 0成功, 1失败 (邀请码不存在, 用户已在店铺中, 数据库错误)
 */
int attend_shop(int user_id, const string& invite_code) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        return 1;
    }
    sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);

    // 1. 查找邀请码对应的商店id
    int shop_id = -1;
    const char* sql_get_id = "SELECT id FROM shops WHERE invite_code = ?;";
    sqlite3_stmt* stmt_get_id;
    if (sqlite3_prepare_v2(db, sql_get_id, -1, &stmt_get_id, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt_get_id, 1, invite_code.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt_get_id) == SQLITE_ROW) {
            shop_id = sqlite3_column_int(stmt_get_id, 0);
        }
        sqlite3_finalize(stmt_get_id);
    }
    if (shop_id == -1) {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return 1; // 邀请码不存在
    }

    // 2. 检查用户是否已是该店成员 (包括经理)
    bool is_member = false;
    const char* sql_check_member = "SELECT 1 FROM seller_shops WHERE seller_id = ? AND shop_id = ? "
                                   "UNION ALL "
                                   "SELECT 1 FROM shops WHERE manager_id = ? AND id = ?;";
    sqlite3_stmt* stmt_check_member;
    if (sqlite3_prepare_v2(db, sql_check_member, -1, &stmt_check_member, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_check_member, 1, user_id);
        sqlite3_bind_int(stmt_check_member, 2, shop_id);
        sqlite3_bind_int(stmt_check_member, 3, user_id);
        sqlite3_bind_int(stmt_check_member, 4, shop_id);
        if (sqlite3_step(stmt_check_member) == SQLITE_ROW) {
            is_member = true;
        }
        sqlite3_finalize(stmt_check_member);
    }
    if (is_member) {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return 1; // 用户已在店铺中
    }

    // 3. 插入到 seller_shops 表
    int result = 1;
    const char* sql_insert = "INSERT INTO seller_shops (seller_id, shop_id) VALUES (?, ?);";
    sqlite3_stmt* stmt_insert;
    if (sqlite3_prepare_v2(db, sql_insert, -1, &stmt_insert, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_insert, 1, user_id);
        sqlite3_bind_int(stmt_insert, 2, shop_id);
        if (sqlite3_step(stmt_insert) == SQLITE_DONE) {
            result = 0;
        }
        sqlite3_finalize(stmt_insert);
    }

    if (result == 0) {
        sqlite3_exec(db, "COMMIT;", 0, 0, 0);
    } else {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
    }
    sqlite3_close(db);
    return result;
}

/**
 * @brief 用户退出商店
 * @param user_id 要退出的用户id
 * @param shop_id 要退出的商店id
 * @return 0成功, 1失败 (用户非该店销售员, 有进行中的订单, 数据库错误)
 */
int leave_shop(int user_id, int shop_id) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        return 1;
    }
    sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);

    // 1. 检查用户是否是该店的销售员 (经理不能通过此API退出)
    bool is_seller = false;
    const char* sql_check_seller = "SELECT 1 FROM seller_shops WHERE seller_id = ? AND shop_id = ?;";
    sqlite3_stmt* stmt_check_seller;
    if (sqlite3_prepare_v2(db, sql_check_seller, -1, &stmt_check_seller, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_check_seller, 1, user_id);
        sqlite3_bind_int(stmt_check_seller, 2, shop_id);
        if (sqlite3_step(stmt_check_seller) == SQLITE_ROW) {
            is_seller = true;
        }
        sqlite3_finalize(stmt_check_seller);
    }
    if (!is_seller) {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return 1; // 用户不是该店销售员
    }
    
    // 2. 检查用户在该商店里是否有进行中的订单
    bool has_ongoing_orders = false;
    const char* sql_check_orders = "SELECT 1 FROM orders WHERE seller_id = ? AND shop_id = ? AND order_status NOT IN (3, 5) LIMIT 1;";
    sqlite3_stmt* stmt_check_orders;
    if (sqlite3_prepare_v2(db, sql_check_orders, -1, &stmt_check_orders, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_check_orders, 1, user_id);
        sqlite3_bind_int(stmt_check_orders, 2, shop_id);
        if (sqlite3_step(stmt_check_orders) == SQLITE_ROW) {
            has_ongoing_orders = true;
        }
        sqlite3_finalize(stmt_check_orders);
    }
    if (has_ongoing_orders) {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return 1; // 存在进行中的订单
    }

    // 3. 从 seller_shops 删除
    int result = 1;
    const char* sql_delete = "DELETE FROM seller_shops WHERE seller_id = ? AND shop_id = ?;";
    sqlite3_stmt* stmt_delete;
    if (sqlite3_prepare_v2(db, sql_delete, -1, &stmt_delete, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_delete, 1, user_id);
        sqlite3_bind_int(stmt_delete, 2, shop_id);
        if (sqlite3_step(stmt_delete) == SQLITE_DONE && sqlite3_changes(db) > 0) {
            result = 0;
        }
        sqlite3_finalize(stmt_delete);
    }

    if (result == 0) {
        sqlite3_exec(db, "COMMIT;", 0, 0, 0);
    } else {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
    }
    sqlite3_close(db);
    return result;
}

/**
 * @brief 获取用户加入或创建的所有商店的id
 * @param user_id 用户id
 * @param id_vec [out] 存放商店id的vector
 * @return 0成功, 1失败 (数据库错误)
 */
int get_user_shop_id(int user_id, vector<int>& id_vec) {
    id_vec.clear();
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        return 1;
    }

    // 1. 获取用户创建的商店
    const char* sql_managed = "SELECT id FROM shops WHERE manager_id = ?;";
    sqlite3_stmt* stmt_managed;
    if (sqlite3_prepare_v2(db, sql_managed, -1, &stmt_managed, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_managed, 1, user_id);
        while (sqlite3_step(stmt_managed) == SQLITE_ROW) {
            id_vec.push_back(sqlite3_column_int(stmt_managed, 0));
        }
        sqlite3_finalize(stmt_managed);
    } else {
        sqlite3_close(db);
        return 1;
    }

    // 2. 获取用户加入的商店
    const char* sql_joined = "SELECT shop_id FROM seller_shops WHERE seller_id = ?;";
    sqlite3_stmt* stmt_joined;
    if (sqlite3_prepare_v2(db, sql_joined, -1, &stmt_joined, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_joined, 1, user_id);
        while (sqlite3_step(stmt_joined) == SQLITE_ROW) {
            id_vec.push_back(sqlite3_column_int(stmt_joined, 0));
        }
        sqlite3_finalize(stmt_joined);
    } else {
        sqlite3_close(db);
        return 1;
    }
    
    sqlite3_close(db);
    return 0;
}

/**
 * @brief 根据id获取商店信息
 * @param id 商店id
 * @param shop [out] 存放商店信息的结构体
 * @return 0成功, 1失败 (id不存在, 数据库错误)
 */
int get_shop_information(int id, ShopInfo& shop) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        return 1;
    }

    // 使用 JOIN 查询一次性获取商店和经理信息
    const char* sql = "SELECT s.id, s.name, s.invite_code, s.manager_id, u.name, s.creation_date, s.status, s.description "
                      "FROM shops s JOIN users u ON s.manager_id = u.id WHERE s.id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        sqlite3_close(db);
        return 1;
    }
    sqlite3_bind_int(stmt, 1, id);

    int result = 1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        auto get_text = [&](int col_idx) {
            const char* text = (const char*)sqlite3_column_text(stmt, col_idx);
            return text ? std::string(text) : "";
        };

        shop.id = sqlite3_column_int(stmt, 0);
        shop.name = get_text(1);
        shop.invite_code = get_text(2);
        shop.manager_id = sqlite3_column_int(stmt, 3);
        shop.manager_name = get_text(4);
        shop.setupDate = get_text(5);
        int status = sqlite3_column_int(stmt, 6);
        shop.state = (status == 1) ? "营业中" : "已关闭";
        shop.description = get_text(7);
        result = 0;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

int user_in_shop(int user_id, int shop_id) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        return 0;
    }

    // 检查用户是否是该店的经理或销售员
    bool is_member = false;
    const char* sql_check_member = "SELECT 1 FROM shops WHERE id = ? AND manager_id = ? "
                                   "UNION ALL "
                                   "SELECT 1 FROM seller_shops WHERE shop_id = ? AND seller_id = ?;";
    sqlite3_stmt* stmt_check_member;
    if (sqlite3_prepare_v2(db, sql_check_member, -1, &stmt_check_member, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_check_member, 1, shop_id);
        sqlite3_bind_int(stmt_check_member, 2, user_id);
        sqlite3_bind_int(stmt_check_member, 3, shop_id);
        sqlite3_bind_int(stmt_check_member, 4, user_id);
        if (sqlite3_step(stmt_check_member) == SQLITE_ROW) {
            is_member = true;
        }
        sqlite3_finalize(stmt_check_member);
    }

    sqlite3_close(db);
    return is_member ? 1 : 0;
}