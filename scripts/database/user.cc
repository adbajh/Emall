#include "database.h"

/**
 * @brief 生成一个指定长度的随机字符串 (a-z, A-Z, 0-9)
 * @param len 期望的字符串长度
 * @return 生成的随机字符串
 */
static std::string generate_random_token(int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string token;
    token.reserve(len);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, sizeof(alphanum) - 2);

    for (int i = 0; i < len; ++i) {
        token += alphanum[distrib(gen)];
    }
    return token;
}

// (2) 用户相关

/**
 * @brief 用户注册账号
 * @param username 用户姓名
 * @param account 用户账号
 * @param password 用户密码
 * @return 0成功，1失败 (账号或密码为空, 账号已存在, 数据库错误)
 */
int user_register(const string& username, const string& account, const string& password) {
    if (account.empty() || password.empty()) {
        return 1; // 失败：账号或密码不为空
    }

    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        return 1;
    }

    const char* sql_insert = "INSERT INTO users (name, account, password) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql_insert, -1, &stmt, 0) != SQLITE_OK) {
        sqlite3_close(db);
        return 1;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, account.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, password.c_str(), -1, SQLITE_TRANSIENT);

    int result = 1;
    // sqlite3_step会因为 UNIQUE 约束失败而返回 SQLITE_CONSTRAINT
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        result = 0; // 成功
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

/**
 * @brief 用户登录
 * @param account 用户账号
 * @param password 用户密码
 * @param token [out] 成功时，传出生成的登录token
 * @return 0成功，1失败 (账号不存在, 密码错误, 数据库错误)
 */
int user_login(const string& account, const string& password, string& token) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        return 1;
    }

    const char* sql_select = "SELECT password FROM users WHERE account = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql_select, -1, &stmt, 0) != SQLITE_OK) {
        sqlite3_close(db);
        return 1;
    }
    sqlite3_bind_text(stmt, 1, account.c_str(), -1, SQLITE_STATIC);

    int result = 1; // 默认为失败
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* db_password = (const char*)sqlite3_column_text(stmt, 0);
        if (db_password && password == db_password) {
            // 密码匹配，生成并更新token
            token = generate_random_token(20);
            const char* sql_update = "UPDATE users SET token = ? WHERE account = ?;";
            sqlite3_stmt* stmt_update;
            if (sqlite3_prepare_v2(db, sql_update, -1, &stmt_update, 0) == SQLITE_OK) {
                sqlite3_bind_text(stmt_update, 1, token.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt_update, 2, account.c_str(), -1, SQLITE_TRANSIENT);
                if (sqlite3_step(stmt_update) == SQLITE_DONE) {
                    result = 0; // 成功
                }
                sqlite3_finalize(stmt_update);
            }
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

/**
 * @brief 用户登出
 * @param account 用户账号
 * @param token 用户当前token
 * @return 0成功，1失败 (账号或token不匹配)
 */
int user_logout(const string& account, const string& token) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        return 1;
    }

    const char* sql = "UPDATE users SET token = NULL WHERE account = ? AND token = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        sqlite3_close(db);
        return 1;
    }

    sqlite3_bind_text(stmt, 1, account.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, token.c_str(), -1, SQLITE_TRANSIENT);

    // 执行更新
    sqlite3_step(stmt);
    
    // 检查是否有行被更改
    int changes = sqlite3_changes(db);
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return (changes > 0) ? 0 : 1;
}

/**
 * @brief 用户删除账号
 * @param account 用户账号
 * @param token 用户当前token
 * @return 0成功，1失败 (认证失败, 存在未完成的订单, 数据库错误)
 */
int user_delete_account(const string& account, const string& token) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        return 1;
    }
    
    // 开启事务以保证操作的原子性
    sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);

    // 1. 认证用户并获取ID
    int user_id = -1;
    const char* sql_get_id = "SELECT id FROM users WHERE account = ? AND token = ?;";
    sqlite3_stmt* stmt_get_id;
    if (sqlite3_prepare_v2(db, sql_get_id, -1, &stmt_get_id, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt_get_id, 1, account.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt_get_id, 2, token.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt_get_id) == SQLITE_ROW) {
            user_id = sqlite3_column_int(stmt_get_id, 0);
        }
        sqlite3_finalize(stmt_get_id);
    }

    if (user_id == -1) {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return 1; // 认证失败
    }

    // 2. 检查用户是否有进行中的订单 (状态不为 3-完成 或 5-无效)
    bool has_ongoing_orders = false;
    const char* sql_check_orders = "SELECT 1 FROM orders WHERE (buyer_id = ? OR seller_id = ?) AND order_status NOT IN (3, 5) LIMIT 1;";
    sqlite3_stmt* stmt_check_orders;
    if (sqlite3_prepare_v2(db, sql_check_orders, -1, &stmt_check_orders, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_check_orders, 1, user_id);
        sqlite3_bind_int(stmt_check_orders, 2, user_id);
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

    // 3. 删除用户 (依赖于外键的 ON DELETE CASCADE 自动清理相关表)
    int result = 1;
    const char* sql_delete = "DELETE FROM users WHERE id = ?;";
    sqlite3_stmt* stmt_delete;
    if (sqlite3_prepare_v2(db, sql_delete, -1, &stmt_delete, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_delete, 1, user_id);
        if (sqlite3_step(stmt_delete) == SQLITE_DONE && sqlite3_changes(db) > 0) {
            result = 0; // 删除成功
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
 * @brief 获取用户在数据库里的id值
 * @param account 用户账号
 * @param token 用户当前token
 * @return 成功则返回用户id，否则返回-1
 */
int user_get_id(const string& account, const string& token) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        return -1;
    }

    const char* sql = "SELECT id FROM users WHERE account = ? AND token = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        sqlite3_close(db);
        return -1;
    }
    sqlite3_bind_text(stmt, 1, account.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, token.c_str(), -1, SQLITE_STATIC);

    int user_id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user_id = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return user_id;
}

/**
 * @brief (重构后) 修改用户的常规信息 (名称、手机、邮箱、描述)
 * @param account 用户当前账号 (用于认证)
 * @param token 用户当前token (用于认证)
 * @param user 包含新信息的结构体 (只会使用 name, phone, email, description 字段)
 * @return 0成功, 1失败 (认证失败)
 */
int user_modify_information(const string& account, const string& token, const UserInfo& user) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        return 1;
    }

    const char* sql_update = "UPDATE users SET name = ?, phone_number = ?, email = ?, description = ? WHERE account = ? AND token = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql_update, -1, &stmt, 0) != SQLITE_OK) {
        sqlite3_close(db);
        return 1;
    }
    
    // 绑定新信息
    sqlite3_bind_text(stmt, 1, user.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user.phone.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, user.email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, user.description.c_str(), -1, SQLITE_TRANSIENT);
    // 绑定认证信息
    sqlite3_bind_text(stmt, 5, account.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, token.c_str(), -1, SQLITE_TRANSIENT);

    int result = 1; // 默认为失败
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        // 检查是否有行被实际更改，如果没有匹配的行，changes会是0
        if (sqlite3_changes(db) > 0) {
            result = 0; // 成功
        }
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}


/**
 * @brief (新增) 修改用户账号
 * @param account 当前账号
 * @param token 当前token
 * @param password 当前密码 (用于二次验证)
 * @param new_account 新的账号
 * @return 0成功, 1失败 (认证失败, 新账号已存在)
 */
int user_modify_account(const string& account, const string& token, const string& password, const string& new_account) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        return 1;
    }

    sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);

    // 1. 验证用户身份 (account, token, password 必须全部匹配)
    int user_id = -1;
    const char* sql_auth = "SELECT id FROM users WHERE account = ? AND token = ? AND password = ?;";
    sqlite3_stmt* stmt_auth;
    if (sqlite3_prepare_v2(db, sql_auth, -1, &stmt_auth, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt_auth, 1, account.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt_auth, 2, token.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt_auth, 3, password.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt_auth) == SQLITE_ROW) {
            user_id = sqlite3_column_int(stmt_auth, 0);
        }
        sqlite3_finalize(stmt_auth);
    }
    
    if (user_id == -1) { // 认证失败
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return 1;
    }
    
    // 2. 检查新账号是否已被占用
    bool new_account_exists = false;
    const char* sql_check = "SELECT 1 FROM users WHERE account = ?;";
    sqlite3_stmt* stmt_check;
    if (sqlite3_prepare_v2(db, sql_check, -1, &stmt_check, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt_check, 1, new_account.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt_check) == SQLITE_ROW) {
            new_account_exists = true;
        }
        sqlite3_finalize(stmt_check);
    }

    if (new_account_exists) { // 新账号已存在
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return 1;
    }
    
    // 3. 执行更新
    int result = 1;
    const char* sql_update = "UPDATE users SET account = ? WHERE id = ?;";
    sqlite3_stmt* stmt_update;
    if (sqlite3_prepare_v2(db, sql_update, -1, &stmt_update, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt_update, 1, new_account.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt_update, 2, user_id);
        if (sqlite3_step(stmt_update) == SQLITE_DONE) {
            result = 0;
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
 * @brief (新增) 修改用户密码
 * @param account 当前账号
 * @param token 当前token
 * @param old_password 旧密码 (用于验证)
 * @param new_password 新密码
 * @return 0成功, 1失败 (认证失败)
 */
int user_modify_password(const string& account, const string& token, const string& old_password, const string& new_password) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        return 1;
    }

    const char* sql_update = "UPDATE users SET password = ? WHERE account = ? AND token = ? AND password = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql_update, -1, &stmt, 0) != SQLITE_OK) {
        sqlite3_close(db);
        return 1;
    }
    
    // 绑定新密码和认证信息
    sqlite3_bind_text(stmt, 1, new_password.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, account.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, token.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, old_password.c_str(), -1, SQLITE_TRANSIENT);

    int result = 1; // 默认为失败
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        // 检查是否有行被实际更改，如果没有匹配的行，changes会是0
        if (sqlite3_changes(db) > 0) {
            result = 0; // 成功
        }
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

/**
 * @brief 获取指定id的用户的信息
 * @param id 要查询的用户id
 * @param user [out] 成功时，用于存储用户信息的结构体
 * @return 0成功，1失败 (用户id不存在)
 */
int get_user_information(int id, UserInfo& user) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        return 1;
    }

    const char* sql = "SELECT id, name, account, password, token, phone_number, email, description FROM users WHERE id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        sqlite3_close(db);
        return 1;
    }
    sqlite3_bind_int(stmt, 1, id);

    int result = 1; // 默认为失败
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user.id = sqlite3_column_int(stmt, 0);
        // 使用 lambda 表达式安全地处理可能为 NULL 的文本字段
        auto get_text = [&](int col_idx) {
            const char* text = (const char*)sqlite3_column_text(stmt, col_idx);
            return text ? std::string(text) : "";
        };
        user.name = get_text(1);
        user.account = get_text(2);
        user.password = get_text(3);
        user.token = get_text(4);
        user.phone = get_text(5);
        user.email = get_text(6);
        user.description = get_text(7);
        result = 0; // 成功
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}