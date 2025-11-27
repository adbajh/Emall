#include "database.h"

// (5) 类别相关

/**
 * @brief 获取数据库中所有的类别名称
 * @param types [out] 用于存储所有类别名称的字符串向量
 * @return 成功返回0，失败返回1
 */
int get_category_information(vector<string>& types) {
    // 首先清空输出向量，确保它不包含旧数据
    types.clear();

    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        // 如果数据库无法打开，直接返回失败
        return 1;
    }

    // SQL 查询语句：从 categories 表中选取所有 name 列，并按字母顺序排序
    const char* sql = "SELECT name FROM categories ORDER BY name;";
    sqlite3_stmt* stmt;

    // 准备 SQL 语句
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        // 如果准备失败，关闭数据库连接并返回失败
        sqlite3_close(db);
        return 1;
    }

    // 循环执行 SQL 语句，获取每一行结果
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        // 从当前行的第0列获取类别名称（const char* 类型）
        const char* category_name = (const char*)sqlite3_column_text(stmt, 0);
        // 将 C 风格字符串转换为 std::string 并添加到向量中
        if (category_name) {
            types.push_back(category_name);
        }
    }

    // 释放语句句柄，清理资源
    sqlite3_finalize(stmt);
    // 关闭数据库连接
    sqlite3_close(db);

    // 函数成功完成
    return 0;
}