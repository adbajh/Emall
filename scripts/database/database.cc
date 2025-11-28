#include "database.h"

const char* database_name = "/home/amax/emall/data/database/database.db";

void initialize_table() {
    // 1. 如果 database_name 存在，则删除该文件
    remove(database_name);

    // 2. 新建一个 .db 文件并打开连接
    sqlite3* db;
    int rc = sqlite3_open(database_name, &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    // 3. 建表
    char* err_msg = nullptr;
    const char* sql_create_tables =
        // (1) users table
        "CREATE TABLE users("
        "id           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name         CHAR(10),"
        "account      CHAR(20) UNIQUE NOT NULL,"
        "password     CHAR(20) NOT NULL,"
        "token        CHAR(20),"
        "phone_number CHAR(13),"
        "email        CHAR(30),"
        "description  VARCHAR(100));"

        // (2) shops table
        "CREATE TABLE shops("
        "id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name          CHAR(10),"
        "invite_code   CHAR(10) UNIQUE NOT NULL,"
        "manager_id    INT NOT NULL,"
        "creation_date DATE,"
        "status        SMALLINT,"
        "description   VARCHAR(100),"
        "FOREIGN KEY(manager_id) REFERENCES users(id) ON DELETE CASCADE);"

        // (3) items table
        "CREATE TABLE items("
        "id           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name         CHAR(10),"
        "seller_id    INT NOT NULL,"
        "shop_id      INT NOT NULL,"
        "price        FLOAT,"
        "publish_date DATE,"
        "publish_time TIME,"
        "description  VARCHAR(100),"
        "FOREIGN KEY(seller_id) REFERENCES users(id) ON DELETE CASCADE,"
        "FOREIGN KEY(shop_id) REFERENCES shops(id) ON DELETE CASCADE);"

        // (4) categories table
        "CREATE TABLE categories("
        "id   INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name CHAR(10) UNIQUE NOT NULL);"

        // (5) item_categories table
        "CREATE TABLE item_categories("
        "id          INTEGER PRIMARY KEY AUTOINCREMENT,"
        "item_id     INT NOT NULL,"
        "category_id INT NOT NULL,"
        "FOREIGN KEY(item_id) REFERENCES items(id) ON DELETE CASCADE,"
        "FOREIGN KEY(category_id) REFERENCES categories(id) ON DELETE CASCADE);"

        // (6) seller_shops table
        "CREATE TABLE seller_shops("
        "id        INTEGER PRIMARY KEY AUTOINCREMENT,"
        "seller_id INT NOT NULL,"
        "shop_id   INT NOT NULL,"
        "FOREIGN KEY(seller_id) REFERENCES users(id) ON DELETE CASCADE,"
        "FOREIGN KEY(shop_id) REFERENCES shops(id) ON DELETE CASCADE,"
        "UNIQUE(seller_id, shop_id));"

        // (7) orders table
        "CREATE TABLE orders("
        "id           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "item_id      INT NOT NULL,"
        "buyer_id     INT NOT NULL,"
        "seller_id    INT NOT NULL,"
        "shop_id      INT NOT NULL,"
        "quantity     INT,"
        "total_price  FLOAT,"
        "address      VARCHAR(50),"
        "order_date   DATE,"
        "order_time   TIME,"
        "order_status TINYINT,"
        "FOREIGN KEY(item_id) REFERENCES items(id) ON DELETE RESTRICT,"
        "FOREIGN KEY(buyer_id) REFERENCES users(id) ON DELETE CASCADE,"
        "FOREIGN KEY(seller_id) REFERENCES users(id) ON DELETE RESTRICT,"
        "FOREIGN KEY(shop_id) REFERENCES shops(id) ON DELETE RESTRICT);"

        // (8) messages table
        "CREATE TABLE messages("
        "id          INTEGER PRIMARY KEY AUTOINCREMENT,"
        "sender_id   INT NOT NULL,"
        "receiver_id INT NOT NULL,"
        "send_date   DATE,"
        "send_time   TIME,"
        "content     VARCHAR(100),"
        "is_read     TINYINT NOT NULL DEFAULT 0," // 新增字段，0=未读, 1=已读
        "FOREIGN KEY(sender_id) REFERENCES users(id) ON DELETE CASCADE,"
        "FOREIGN KEY(receiver_id) REFERENCES users(id) ON DELETE CASCADE);";

    rc = sqlite3_exec(db, sql_create_tables, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error in initialize: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    }

    std::cout << "Tables have been successfully initialized." << std::endl;

    // 4. 关闭数据库连接
    sqlite3_close(db);
}

void initialize_case() {
    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        std::cerr << "Cannot open database for test cases: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    char* err_msg = nullptr;
    const char* sql_insert_data =
        "BEGIN TRANSACTION;"

        // --- (1) 添加6个用户 ---
        // ID=1, Alice: 经理, 有两家店
        "INSERT INTO users (name, account, password, description) VALUES ('Alice', 'alice123', 'pass1', '我管理着两家很棒的店铺！');"
        // ID=2, Bob: 销售员, 加入了Alice的两家店
        "INSERT INTO users (name, account, password, description) VALUES ('Bob', 'bob456', 'pass2', '我是一名金牌销售。');"
        // ID=3, Charlie: 纯买家
        "INSERT INTO users (name, account, password, description) VALUES ('Charlie', 'charlie789', 'pass3', '我喜欢购物。');"
        // ID=4, Diana: 经理, 有一家自己的店
        "INSERT INTO users (name, account, password, description) VALUES ('Diana', 'diana001', 'pass4', '家居生活，找我就对啦。');"
        // ID=5, Eve: 销售员, 加入了Diana的店
        "INSERT INTO users (name, account, password, description) VALUES ('Eve', 'eve002', 'pass5', '欢迎光临我的小店~');"
        // ID=6, Frank: 纯买家
        "INSERT INTO users (name, account, password, description) VALUES ('Frank', 'frank003', 'pass6', '正在寻找一些好东西。');"

        // --- (2) 创建3家店铺 ---
        // ID=1, Alice(1)的科技店
        "INSERT INTO shops (name, invite_code, manager_id, creation_date, status, description) VALUES ('Alice科技', 'TECH01', 1, date('now', '-20 days'), 1, '最新最酷的电子产品。');"
        // ID=2, Alice(1)的书店
        "INSERT INTO shops (name, invite_code, manager_id, creation_date, status, description) VALUES ('书虫天堂', 'BOOK02', 1, date('now', '-15 days'), 1, '知识的海洋，精神的家园。');"
        // ID=3, Diana(4)的家居店
        "INSERT INTO shops (name, invite_code, manager_id, creation_date, status, description) VALUES ('戴安娜家居', 'HOME03', 4, date('now', '-10 days'), 1, '为您的家增添一份温馨。');"

        // --- (3) 建立销售员与店铺的关系 ---
        "INSERT INTO seller_shops (seller_id, shop_id) VALUES (2, 1);" // Bob(2) 加入 Alice科技(1)
        "INSERT INTO seller_shops (seller_id, shop_id) VALUES (2, 2);" // Bob(2) 也加入了 书虫天堂(2)
        "INSERT INTO seller_shops (seller_id, shop_id) VALUES (5, 3);" // Eve(5) 加入 戴安娜家居(3)

        // --- (4) 发布多种商品 ---
        // 经理Alice(1)在她自己的店里发布商品
        "INSERT INTO items (name, seller_id, shop_id, price, description, publish_date) VALUES ('游戏本', 1, 1, 8999.00, '高性能电竞笔记本', date('now', '-5 days'));" // item_id=1
        "INSERT INTO items (name, seller_id, shop_id, price, description, publish_date) VALUES ('智能手表', 1, 1, 1299.00, '健康监测，运动伴侣', date('now', '-4 days'));" // item_id=2
        "INSERT INTO items (name, seller_id, shop_id, price, description, publish_date) VALUES ('历史巨著', 1, 2, 88.50, '一部深刻的世界史', date('now', '-3 days'));"   // item_id=3
        // 销售员Bob(2)在他加入的店里发布商品
        "INSERT INTO items (name, seller_id, shop_id, price, description, publish_date) VALUES ('机械键盘', 2, 1, 499.00, '手感超群，RGB光效', date('now', '-2 days'));" // item_id=4
        "INSERT INTO items (name, seller_id, shop_id, price, description, publish_date) VALUES ('科幻漫画', 2, 2, 25.00, '畅销系列最终卷', date('now', '-1 day'));"    // item_id=5
        // 经理Diana(4)在她自己的店里发布商品
        "INSERT INTO items (name, seller_id, shop_id, price, description, publish_date) VALUES ('布艺沙发', 4, 3, 2599.00, '三人位，舒适柔软', date('now', '-2 days'));" // item_id=6
        // 销售员Eve(5)在她加入的店里发布商品
        "INSERT INTO items (name, seller_id, shop_id, price, description, publish_date) VALUES ('装饰台灯', 5, 3, 189.00, '简约设计，温馨光线', date('now', '-1 day'));"   // item_id=7

        // --- (5) 关联商品和类别 (注意：这里的category_id需要与initialize_category中的插入顺序对应) ---
        "INSERT INTO item_categories (item_id, category_id) VALUES (1, 2);"  // 游戏本 -> 电脑整机
        "INSERT INTO item_categories (item_id, category_id) VALUES (1, 9);"  // 游戏本 -> 游戏设备
        "INSERT INTO item_categories (item_id, category_id) VALUES (2, 8);"  // 智能手表 -> 智能穿戴
        "INSERT INTO item_categories (item_id, category_id) VALUES (3, 24);" // 历史巨著 -> 图书
        "INSERT INTO item_categories (item_id, category_id) VALUES (4, 3);"  // 机械键盘 -> 电脑配件
        "INSERT INTO item_categories (item_id, category_id) VALUES (5, 24);" // 科幻漫画 -> 图书
        "INSERT INTO item_categories (item_id, category_id) VALUES (5, 29);" // 科幻漫画 -> 动漫周边
        "INSERT INTO item_categories (item_id, category_id) VALUES (6, 20);" // 布艺沙发 -> 家具
        "INSERT INTO item_categories (item_id, category_id) VALUES (7, 23);" // 装饰台灯 -> 灯具

        // --- (6) 创建多个不同状态的订单 ---
        // 订单1: 已完成。Charlie(3) 购买了 Bob(2) 的 机械键盘(4)
        "INSERT INTO orders (item_id, buyer_id, seller_id, shop_id, quantity, total_price, address, order_date, order_status) VALUES (4, 3, 2, 1, 1, 499.00, '南京市仙林大道163号', date('now', '-1 day'), 3);"
        // 订单2: 已支付，待发货。Frank(6) 购买了 Alice(1) 的 智能手表(2)
        "INSERT INTO orders (item_id, buyer_id, seller_id, shop_id, quantity, total_price, address, order_date, order_status) VALUES (2, 6, 1, 1, 1, 1299.00, '上海市浦东新区', date('now'), 1);"
        // 订单3: 已发货，待收货。Charlie(3) 购买了 Diana(4) 的 布艺沙发(6)
        "INSERT INTO orders (item_id, buyer_id, seller_id, shop_id, quantity, total_price, address, order_date, order_status) VALUES (6, 3, 4, 3, 1, 2599.00, '南京市仙林大道163号', date('now'), 2);"
        // 订单4: 未支付。Frank(6) 购买了 Eve(5) 的 2个台灯(7)
        "INSERT INTO orders (item_id, buyer_id, seller_id, shop_id, quantity, total_price, address, order_date, order_status) VALUES (7, 6, 5, 3, 2, 378.00, '上海市浦东新区', date('now'), 0);"

        // --- (7) 创建一些消息，包含未读消息 ---
        // Charlie(3) 问 Diana(4) 关于沙发的问题
        "INSERT INTO messages (sender_id, receiver_id, content, send_date, send_time, is_read) VALUES (3, 4, '你好，请问这个沙发是什么材质的？', date('now'), '10:30:00', 1);"
        // Diana(4) 回复了 Charlie(3)
        "INSERT INTO messages (sender_id, receiver_id, content, send_date, send_time, is_read) VALUES (4, 3, '亲，是高弹海绵和棉麻布料的哦', date('now'), '10:32:00', 0);" // Charlie未读
        // 经理间的对话
        "INSERT INTO messages (sender_id, receiver_id, content, send_date, send_time, is_read) VALUES (1, 4, 'Diana, 你的家居店看起来真不错！', date('now', '-1 day'), '09:00:00', 1);"
        "INSERT INTO messages (sender_id, receiver_id, content, send_date, send_time, is_read) VALUES (4, 1, '谢谢你Alice，有空来坐坐呀！', date('now', '-1 day'), '09:05:00', 1);"
        // Frank(6) 给 Bob(2) 发消息，Bob未读
        "INSERT INTO messages (sender_id, receiver_id, content, send_date, send_time, is_read) VALUES (6, 2, '老板，键盘还有其他颜色吗？', date('now'), '11:00:00', 0);"

        "COMMIT;";

    if (sqlite3_exec(db, sql_insert_data, 0, 0, &err_msg) != SQLITE_OK) {
        std::cerr << "SQL error in initialize_case: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    } else {
        std::cout << "Database has been successfully initialized with extensive test cases." << std::endl;
    }

    sqlite3_close(db);
}

void initialize_category() {
    // 预设的商品分类列表
    const std::vector<std::string> categories = {
        // --- 电子产品 (Electronics) ---
        "手机平板", "电脑整机", "电脑配件", "摄影摄像", "影音娱乐", "家用电器", "厨房电器", "智能穿戴", "游戏设备",

        // --- 服饰鞋包 (Apparel & Accessories) ---
        "男装", "女装", "童装", "内衣", "鞋靴", "箱包", "奢侈品", "珠宝首饰", "钟表", "配饰",

        // --- 美妆护肤 (Beauty & Skincare) ---
        "面部护肤", "彩妆香水", "身体护理", "美发护发", "美容工具",

        // --- 家居生活 (Home & Living) ---
        "家具", "家纺", "厨具", "灯具", "收纳整理", "家居装饰", "宠物生活",

        // --- 图书文娱 (Books & Entertainment) ---
        "图书", "文具", "乐器", "影视", "音乐", "动漫周边",

        // --- 运动户外 (Sports & Outdoors) ---
        "运动服饰", "健身训练", "体育用品", "户外装备", "骑行运动",

        // --- 食品生鲜 (Groceries & Fresh Food) ---
        "休闲零食", "生鲜水果", "蔬菜", "粮油调味", "酒水饮料", "茗茶", "营养保健",

        // --- 母婴用品 (Maternity & Baby) ---
        "奶粉辅食", "尿裤湿巾", "童车童床", "喂养用品", "孕妈专区",

        // --- 汽车用品 (Automotive) ---
        "车载电器", "汽车装饰", "维修保养", "安全自驾",

        // --- 其他 (Miscellaneous) ---
        "办公设备", "礼品", "鲜花", "计生情趣", "其它"
    };

    sqlite3* db;
    if (sqlite3_open(database_name, &db) != SQLITE_OK) {
        std::cerr << "Cannot open database in initialize_category: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    char* err_msg = nullptr;

    // 1. 先清空 categories 表，防止重复插入
    const char* sql_delete = "DELETE FROM categories;";
    if (sqlite3_exec(db, sql_delete, 0, 0, &err_msg) != SQLITE_OK) {
        std::cerr << "SQL error clearing categories table: " << err_msg << std::endl;
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return;
    }
    // 重置自增ID计数器
    const char* sql_reset_seq = "DELETE FROM sqlite_sequence WHERE name='categories';";
    sqlite3_exec(db, sql_reset_seq, 0, 0, &err_msg);


    // 2. 使用事务批量插入，效率更高
    sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, 0);

    const char* sql_insert = "INSERT INTO categories (name) VALUES (?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql_insert, -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement in initialize_category: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_close(db);
        return;
    }

    // 3. 循环插入所有分类
    for (const auto& category_name : categories) {
        sqlite3_bind_text(stmt, 1, category_name.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "Failed to insert category: " << category_name << std::endl;
            // 如果某一个插入失败，则回滚整个事务
            sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return;
        }
        sqlite3_reset(stmt); // 重置语句以便下次绑定
    }

    // 4. 提交事务
    sqlite3_exec(db, "COMMIT;", 0, 0, 0);

    // 5. 清理资源
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    std::cout << "Categories have been successfully initialized." << std::endl;
}

void initialize_db() {
    initialize_table();
    initialize_category();
    initialize_case();
}