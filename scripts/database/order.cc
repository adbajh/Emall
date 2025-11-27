#include "database.h"

// (6) 订单相关

/**
 * @brief 创建订单
 * @param buyer_id 买家用户id
 * @param item_id 商品id
 * @param quantity 购买数量
 * @param address 收货地址
 * @param paid 是否已支付
 * @return 成功返回0, 失败返回1 (商品或买家不存在, 数据库错误)
 */
int create_order(int buyer_id, int item_id, int quantity, string address, bool paid) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        return 1;
    }

    sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);

    // 1. 获取商品信息 (卖家id, 店铺id, 价格)
    int seller_id = -1, shop_id = -1;
    float price = 0.0;
    const char* sql_get_item = "SELECT seller_id, shop_id, price FROM items WHERE id = ?;";
    sqlite3_stmt* stmt_get_item;
    if (sqlite3_prepare_v2(db, sql_get_item, -1, &stmt_get_item, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_get_item, 1, item_id);
        if (sqlite3_step(stmt_get_item) == SQLITE_ROW) {
            seller_id = sqlite3_column_int(stmt_get_item, 0);
            shop_id = sqlite3_column_int(stmt_get_item, 1);
            price = sqlite3_column_double(stmt_get_item, 2);
        }
        sqlite3_finalize(stmt_get_item);
    }

    if (seller_id == -1) { // 商品不存在
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return 1;
    }

    // 2. 计算总价并确定初始状态
    float total_price = price * quantity;
    int initial_status = paid ? 1 : 0; // paid=true -> 1 (已支付), paid=false -> 0 (未支付)

    // 3. 插入订单记录
    int result = 1;
    const char* sql_insert = "INSERT INTO orders (item_id, buyer_id, seller_id, shop_id, quantity, total_price, address, order_date, order_time, order_status) "
                           "VALUES (?, ?, ?, ?, ?, ?, ?, date('now'), time('now'), ?);";
    sqlite3_stmt* stmt_insert;
    if (sqlite3_prepare_v2(db, sql_insert, -1, &stmt_insert, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_insert, 1, item_id);
        sqlite3_bind_int(stmt_insert, 2, buyer_id);
        sqlite3_bind_int(stmt_insert, 3, seller_id);
        sqlite3_bind_int(stmt_insert, 4, shop_id);
        sqlite3_bind_int(stmt_insert, 5, quantity);
        sqlite3_bind_double(stmt_insert, 6, total_price);
        sqlite3_bind_text(stmt_insert, 7, address.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt_insert, 8, initial_status);

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
 * @brief 根据用户角色和订单状态获取订单列表
 * @param user_id 用户的id
 * @param is_buyer 用户是作为买家(true)还是卖家(false)查询
 * @param state 要查询的订单状态
 * @param orders [out] 存放所有相关订单信息的向量
 * @return 成功返回0, 失败返回1
 */
int get_orders(int user_id, bool is_buyer, int state, vector<Order> &orders) {
    // 1. 清空输出向量，确保它不包含旧数据
    orders.clear();

    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        return 1; // 数据库打开失败
    }

    // 2. 根据用户角色构建 SQL 查询语句
    // 我们需要 JOIN 多张表来获取所有名称信息
    std::string sql_query_base = 
        "SELECT o.id, o.item_id, i.name, o.buyer_id, u_buyer.name, o.seller_id, u_seller.name, "
        "o.shop_id, s.name, o.quantity, o.total_price, o.address, o.order_date, o.order_time, o.order_status "
        "FROM orders o "
        "JOIN items i ON o.item_id = i.id "
        "JOIN users u_buyer ON o.buyer_id = u_buyer.id "
        "JOIN users u_seller ON o.seller_id = u_seller.id "
        "JOIN shops s ON o.shop_id = s.id ";
    
    std::string sql_query;
    if (is_buyer) {
        sql_query = sql_query_base + "WHERE o.buyer_id = ? AND o.order_status = ? ORDER BY o.order_date DESC, o.order_time DESC;";
    } else {
        sql_query = sql_query_base + "WHERE o.seller_id = ? AND o.order_status = ? ORDER BY o.order_date DESC, o.order_time DESC;";
    }

    // 3. 准备和执行查询
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql_query.c_str(), -1, &stmt, 0) != SQLITE_OK) {
        sqlite3_close(db);
        return 1;
    }

    // 绑定参数：第一个参数是 user_id，第二个是 state
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, state);

    // 4. 遍历查询结果并填充 vector
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Order current_order;

        // 使用 lambda 表达式安全地处理可能为 NULL 的文本字段
        auto get_text = [&](int col_idx) {
            const char* text = (const char*)sqlite3_column_text(stmt, col_idx);
            return text ? std::string(text) : "";
        };

        // 按照 SELECT 语句的顺序，用列索引填充结构体
        current_order.id = sqlite3_column_int(stmt, 0);
        current_order.item_id = sqlite3_column_int(stmt, 1);
        current_order.item_name = get_text(2);
        current_order.buyer_id = sqlite3_column_int(stmt, 3);
        current_order.buyer_name = get_text(4);
        current_order.seller_id = sqlite3_column_int(stmt, 5);
        current_order.seller_name = get_text(6);
        current_order.shop_id = sqlite3_column_int(stmt, 7);
        current_order.shop_name = get_text(8);
        current_order.quantity = sqlite3_column_int(stmt, 9);
        current_order.total_price = sqlite3_column_double(stmt, 10);
        current_order.address = get_text(11);
        current_order.createDate = get_text(12);
        current_order.createTime = get_text(13);
        current_order.state = sqlite3_column_int(stmt, 14);
        
        orders.push_back(current_order);
    }

    // 5. 清理资源并关闭数据库
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 0; // 成功
}

/**
 * @brief 修改订单状态
 * @param user_id 操作者用户id
 * @param order_id 要修改的订单id
 * @param is_buyer 操作者是否为买家
 * @param op 操作指令 (例如 "pay", "ship", "confirm_receipt", "request_refund", "approve_refund", "cancel")
 * @return 成功返回0, 失败返回1 (订单不存在, 无权限, 操作无效)
 */
int edit_order_state(int user_id, int order_id, bool is_buyer, string op) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        return 1;
    }
    sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);

    // 1. 获取订单当前状态和相关方ID
    int current_status = -1, db_buyer_id = -1, db_seller_id = -1;
    const char* sql_get = "SELECT buyer_id, seller_id, order_status FROM orders WHERE id = ?;";
    sqlite3_stmt* stmt_get;
    if (sqlite3_prepare_v2(db, sql_get, -1, &stmt_get, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_get, 1, order_id);
        if (sqlite3_step(stmt_get) == SQLITE_ROW) {
            db_buyer_id = sqlite3_column_int(stmt_get, 0);
            db_seller_id = sqlite3_column_int(stmt_get, 1);
            current_status = sqlite3_column_int(stmt_get, 2);
        }
        sqlite3_finalize(stmt_get);
    }

    if (current_status == -1) { // 订单不存在
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return 1;
    }

    // 2. 验证操作者身份
    if ((is_buyer && user_id != db_buyer_id) || (!is_buyer && user_id != db_seller_id)) {
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return 1; // 无权限
    }

    // 3. 根据当前状态和操作，确定新状态 (状态机)
    int new_status = -1; // -1 表示无效操作
    if (is_buyer) {
        if (op == "pay" && current_status == 0) new_status = 1;               // 支付
        else if (op == "confirm" && current_status == 2) new_status = 3; // 确认收货
        else if (op == "return" && (current_status == 1 || current_status == 2)) new_status = 4; // 申请退款
        else if (op == "cancel" && current_status == 0) new_status = 5;       // 取消未支付订单
    } else { // is_seller
        if (op == "send_out" && current_status == 1) new_status = 2;              // 发货
        else if (op == "refund" && current_status == 4) new_status = 5; // 同意退款
        else if (op == "cancel" && current_status == 0) new_status = 5;      // (卖家)取消未支付订单
    }

    if (new_status == -1) { // 操作对当前状态无效
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return 1;
    }

    // 4. 更新订单状态
    int result = 1;
    const char* sql_update = "UPDATE orders SET order_status = ? WHERE id = ?;";
    sqlite3_stmt* stmt_update;
    if (sqlite3_prepare_v2(db, sql_update, -1, &stmt_update, 0) == SQLITE_OK) {
        sqlite3_bind_int(stmt_update, 1, new_status);
        sqlite3_bind_int(stmt_update, 2, order_id);
        if (sqlite3_step(stmt_update) == SQLITE_DONE && sqlite3_changes(db) > 0) {
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