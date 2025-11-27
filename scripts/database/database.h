#ifndef DATABASE_API_H
#define DATABASE_API_H

#include <bits/stdc++.h>
#include <sqlite3.h>

using namespace std;

extern const char* database_name;

struct UserInfo {
    int id;
    string name;
    string account;
    string password;
    string token;
    string phone;
    string email;
    string description;
};

struct ShopInfo {
    int id;
    string name;
    string invite_code;
    int manager_id;
    string manager_name;
    string setupDate;
    string state;
    string description;
};

struct Item {
    int id;
    string name;
    int seller_id;
    string seller_name;
    int shop_id;
    string shop_name;
    float price;
    string publishDate;
    string publishTime;
    string description;
    vector<string> types;
};

struct Order {
    int id;
    int item_id;
    string item_name;
    int buyer_id;
    string buyer_name;
    int seller_id;
    string seller_name;
    int shop_id;
    string shop_name;
    int quantity;
    float total_price;
    string address;
    string createDate;
    string createTime;
    int state;
};

struct Message {
    int id;
    int sender_id;
    string sender_name;
    int receiver_id;
    string receiver_name;
    string sendDate;
    string sendTime;
    string content;
};

// (1) 初始化

void initialize_db();
void initialize_case();

// (2) 用户相关

int user_register(const string& username, const string& account, const string& password);
int user_login(const string& account, const string& password, string& token);
int user_logout(const string& account, const string& token);
int user_delete_account(const string& account, const string& token);
int user_get_id(const string& account, const string& token);
int user_modify_information(const string& account, const string& token, const UserInfo& user);
int user_modify_account(const string& account, const string& token, const string& password, const string& new_account);
int user_modify_password(const string& account, const string& token, const string& old_password, const string& new_password);
int get_user_information(int id, UserInfo& user);

// (3) 店铺相关

int create_shop(int user_id, const ShopInfo& shop);
int modify_shop_information(int user_id, int shop_id, const ShopInfo& shop);
int delete_shop(int user_id, int shop_id);
int get_shop_id(const string& invite_code);
int attend_shop(int user_id, const string& invite_code);
int leave_shop(int user_id, int shop_id);
int get_user_shop_id(int user_id, vector<int>& id_vec);
int get_shop_information(int id, ShopInfo& shop);
int user_in_shop(int user_id, int shop_id);

// (4) 商品相关

int publish_item(int user_id, int shop_id, const Item& item);
int modify_item_information(int user_id, int shop_id, int item_id, const Item& item);
int delete_item(int user_id, int shop_id, int item_id);
int get_user_shop_item_id(int user_id, int shop_id, vector<int>& id_vec);
int search_next_item_id(int cur_id, map<string, string> cond);
int get_item_information(int id, Item& item);

// (5) 类别相关

int get_category_information(vector<string>& types);

// (6) 订单相关

int create_order(int buyer_id, int item_id, int quantity, string address, bool paid);
int get_orders(int user_id, bool is_buyer, int state, vector<Order> &orders);
int edit_order_state(int user_id, int order_id, bool is_buyer, string op); // 注意：根据上下文推断，添加了 order_id

// (7) 信息相关

struct ContactStat {
    int user_id;
    int unread_count;
    std::string last_datetime; // 格式: "YYYY-MM-DD HH:MM:SS" 用于排序
    std::string last_content;  // (可选) 用于显示最后一条消息预览
};

int get_chat_contacts(int my_id, std::vector<ContactStat>& stats);
/* 
   对应的 SQL (SQLite 语法):
   -------------------------------------------------------
   SELECT 
       other_id,
       SUM(CASE WHEN receiver_id = ? AND is_read = 0 THEN 1 ELSE 0 END) as unread_count,
       MAX(send_date || ' ' || send_time) as last_interaction
   FROM (
       -- 找出我发出的消息 (other_id 是 receiver)
       SELECT receiver_id as other_id, receiver_id, is_read, send_date, send_time 
       FROM messages WHERE sender_id = ?
       UNION ALL
       -- 找出我收到的消息 (other_id 是 sender)
       SELECT sender_id as other_id, receiver_id, is_read, send_date, send_time 
       FROM messages WHERE receiver_id = ?
   )
   GROUP BY other_id
   ORDER BY last_interaction DESC;
   -------------------------------------------------------
   参数顺序: my_id (判断未读), my_id (查发出), my_id (查收到)
*/

int save_message(int sender_id, int receiver_id, const std::string& content, const std::string& date, const std::string& time);
/* 对应的 SQL:
   INSERT INTO messages (sender_id, receiver_id, content, send_date, send_time, is_read) VALUES (?, ?, ?, ?, ?, 0)
*/

int get_latest_messages(int sender_id, int receiver_id, const std::string& last_date, const std::string& last_time, std::vector<Message>& msgs);
/* 对应的 SQL:
   SELECT * FROM messages 
   WHERE sender_id = ? AND receiver_id = ? 
   AND (send_date > ? OR (send_date = ? AND send_time > ?))
   ORDER BY send_date ASC, send_time ASC
*/


int get_history_messages_paged(int my_id, int other_id, const std::string& before_date, const std::string& before_time, int limit, std::vector<Message>& msgs);
/* 对应的 SQL 实现逻辑:
   SELECT * FROM messages 
   WHERE 
     ((sender_id = ? AND receiver_id = ?) OR (sender_id = ? AND receiver_id = ?))
     AND 
     (send_date < ? OR (send_date = ? AND send_time < ?))  -- 关键：只查这个时间之前的
   ORDER BY send_date DESC, send_time DESC                 -- 倒序排，取最近的
   LIMIT ?
*/

int mark_messages_as_read(int my_id, int other_id);
// 对应的 SQL: 标记已读
/*
   UPDATE messages 
   SET is_read = 1 
   WHERE sender_id = ? AND receiver_id = ? AND is_read = 0;
   -- 参数: other_id (对方), my_id (我)
*/

# endif