#include "database.h"
#include <sstream> // for building SQL string

// 辅助函数，用于填充 Message 结构体（避免代码重复）
static void fill_message_from_stmt(sqlite3_stmt* stmt, std::vector<Message>& msgs) {
    auto get_text = [&](int col_idx) {
        const char* text = (const char*)sqlite3_column_text(stmt, col_idx);
        return text ? std::string(text) : "";
    };
    Message msg;
    msg.id = sqlite3_column_int(stmt, 0);
    msg.sender_id = sqlite3_column_int(stmt, 1);
    msg.sender_name = get_text(2);
    msg.receiver_id = sqlite3_column_int(stmt, 3);
    msg.receiver_name = get_text(4);
    msg.sendDate = get_text(5);
    msg.sendTime = get_text(6);
    msg.content = get_text(7);
    msgs.push_back(msg);
}

/**
 * @brief 获取历史联系人列表，包含未读消息数和最后交互时间
 * @param my_id 当前用户的ID
 * @param stats [out] 用于存储联系人统计信息的向量
 * @return 成功返回0, 失败返回1
 */
int get_chat_contacts(int my_id, std::vector<ContactStat>& stats) {
    stats.clear();
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) return 1;

    // 主查询：获取联系人ID，未读数，最后交互时间
    const char* sql_main = 
       "SELECT other_id, SUM(unread) as unread_count, MAX(datetime) as last_datetime "
       "FROM ("
           "SELECT receiver_id as other_id, 0 as unread, (send_date || ' ' || send_time) as datetime FROM messages WHERE sender_id = ? "
           "UNION ALL "
           "SELECT sender_id as other_id, CASE WHEN is_read = 0 THEN 1 ELSE 0 END as unread, (send_date || ' ' || send_time) as datetime FROM messages WHERE receiver_id = ? "
       ") GROUP BY other_id ORDER BY last_datetime DESC;";

    sqlite3_stmt* stmt_main;
    if (sqlite3_prepare_v2(db, sql_main, -1, &stmt_main, 0) != SQLITE_OK) {
        sqlite3_close(db);
        return 1;
    }
    sqlite3_bind_int(stmt_main, 1, my_id);
    sqlite3_bind_int(stmt_main, 2, my_id);

    // 辅助查询：用于获取最后一条消息内容
    const char* sql_content = "SELECT content FROM messages WHERE (sender_id = ? AND receiver_id = ?) OR (sender_id = ? AND receiver_id = ?) ORDER BY send_date DESC, send_time DESC LIMIT 1;";
    sqlite3_stmt* stmt_content;
    if (sqlite3_prepare_v2(db, sql_content, -1, &stmt_content, 0) != SQLITE_OK) {
        sqlite3_finalize(stmt_main);
        sqlite3_close(db);
        return 1;
    }

    while (sqlite3_step(stmt_main) == SQLITE_ROW) {
        ContactStat stat;
        stat.user_id = sqlite3_column_int(stmt_main, 0);
        stat.unread_count = sqlite3_column_int(stmt_main, 1);
        stat.last_datetime = (const char*)sqlite3_column_text(stmt_main, 2);
        
        // 执行辅助查询来获取最后一条消息内容
        sqlite3_bind_int(stmt_content, 1, my_id);
        sqlite3_bind_int(stmt_content, 2, stat.user_id);
        sqlite3_bind_int(stmt_content, 3, stat.user_id);
        sqlite3_bind_int(stmt_content, 4, my_id);
        if (sqlite3_step(stmt_content) == SQLITE_ROW) {
            stat.last_content = (const char*)sqlite3_column_text(stmt_content, 0);
        } else {
            stat.last_content = "";
        }
        sqlite3_reset(stmt_content); // 重置辅助查询以备下次循环使用

        stats.push_back(stat);
    }

    sqlite3_finalize(stmt_content);
    sqlite3_finalize(stmt_main);
    sqlite3_close(db);
    return 0;
}

/**
 * @brief 保存一条新消息到数据库
 * @param sender_id 发送者ID
 * @param receiver_id 接收者ID
 * @param content 消息内容
 * @param date 发送日期 ("YYYY-MM-DD")
 * @param time 发送时间 ("HH:MM:SS")
 * @return 成功返回0, 失败返回1
 */
int save_message(int sender_id, int receiver_id, const std::string& content, const std::string& date, const std::string& time) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) return 1;

    const char* sql = "INSERT INTO messages (sender_id, receiver_id, content, send_date, send_time, is_read) VALUES (?, ?, ?, ?, ?, 0);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        sqlite3_close(db);
        return 1;
    }
    sqlite3_bind_int(stmt, 1, sender_id);
    sqlite3_bind_int(stmt, 2, receiver_id);
    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, time.c_str(), -1, SQLITE_TRANSIENT);

    int result = (sqlite3_step(stmt) == SQLITE_DONE) ? 0 : 1;
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

/**
 * @brief 获取与某人（对方发给我）的、晚于指定时间的最新消息
 * @param sender_id 对方的ID
 * @param receiver_id 我的ID
 * @param last_date 上次轮询的最后日期
 * @param last_time 上次轮询的最后时间
 * @param msgs [out] 存储新消息的向量
 * @return 成功返回0, 失败返回1
 */
int get_latest_messages(int sender_id, int receiver_id, const std::string& last_date, const std::string& last_time, std::vector<Message>& msgs) {
    msgs.clear();
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) return 1;

    const char* sql = "SELECT m.id, m.sender_id, u_sender.name, m.receiver_id, u_receiver.name, m.send_date, m.send_time, m.content "
                      "FROM messages m JOIN users u_sender ON m.sender_id = u_sender.id JOIN users u_receiver ON m.receiver_id = u_receiver.id "
                      "WHERE m.sender_id = ? AND m.receiver_id = ? AND (m.send_date > ? OR (m.send_date = ? AND m.send_time > ?)) "
                      "ORDER BY m.send_date ASC, m.send_time ASC;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        sqlite3_close(db);
        return 1;
    }
    sqlite3_bind_int(stmt, 1, sender_id);
    sqlite3_bind_int(stmt, 2, receiver_id);
    sqlite3_bind_text(stmt, 3, last_date.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, last_date.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, last_time.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        fill_message_from_stmt(stmt, msgs);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

/**
 * @brief 分页获取与某人的历史消息
 * @param my_id 我的ID
 * @param other_id 对方的ID
 * @param before_date 时间上限（日期）
 * @param before_time 时间上限（时间）
 * @param limit 需要获取的消息条数
 * @param msgs [out] 存储历史消息的向量（按时间从早到晚排序）
 * @return 成功返回0, 失败返回1
 */
int get_history_messages_paged(int my_id, int other_id, const std::string& before_date, const std::string& before_time, int limit, std::vector<Message>& msgs) {
    msgs.clear();
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) return 1;
    
    const char* sql = "SELECT m.id, m.sender_id, u_sender.name, m.receiver_id, u_receiver.name, m.send_date, m.send_time, m.content "
                      "FROM messages m JOIN users u_sender ON m.sender_id = u_sender.id JOIN users u_receiver ON m.receiver_id = u_receiver.id "
                      "WHERE ((m.sender_id = ? AND m.receiver_id = ?) OR (m.sender_id = ? AND m.receiver_id = ?)) "
                      "AND (m.send_date < ? OR (m.send_date = ? AND m.send_time < ?)) "
                      "ORDER BY m.send_date DESC, m.send_time DESC LIMIT ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        sqlite3_close(db);
        return 1;
    }
    sqlite3_bind_int(stmt, 1, my_id);
    sqlite3_bind_int(stmt, 2, other_id);
    sqlite3_bind_int(stmt, 3, other_id);
    sqlite3_bind_int(stmt, 4, my_id);
    sqlite3_bind_text(stmt, 5, before_date.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, before_date.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, before_time.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 8, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        fill_message_from_stmt(stmt, msgs);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    // 查询结果是按时间倒序的 (最新的在最前面)，UI显示通常需要正序 (最旧的在最前面)
    std::reverse(msgs.begin(), msgs.end());
    
    return 0;
}

/**
 * @brief 将我收到的、来自对方的所有未读消息标记为已读
 * @param my_id 我的ID
 * @param other_id 对方的ID (即消息的发送方)
 * @return 成功返回0, 失败返回1
 */
int mark_messages_as_read(int my_id, int other_id) {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) return 1;

    // 标记的是对方(sender)发给我(receiver)的未读消息
    const char* sql = "UPDATE messages SET is_read = 1 WHERE sender_id = ? AND receiver_id = ? AND is_read = 0;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        sqlite3_close(db);
        return 1;
    }
    sqlite3_bind_int(stmt, 1, other_id); // sender
    sqlite3_bind_int(stmt, 2, my_id);    // receiver

    int result = (sqlite3_step(stmt) == SQLITE_DONE) ? 0 : 1;
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}
