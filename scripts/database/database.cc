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
    // 1. 调用 initialize() 来确保一个干净的数据库环境
    // initialize();

    // 2. 打开数据库连接
    sqlite3* db;
    int rc = sqlite3_open(database_name, &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database for test cases: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    // 3. 插入测试数据
    char* err_msg = nullptr;
    const char* sql_insert_data =
        // 添加3个用户
        "INSERT INTO users (name, account, password, description) VALUES ('Alice', 'alice123', 'pass1', 'I am a manager.');"
        "INSERT INTO users (name, account, password, description) VALUES ('Bob', 'bob456', 'pass2', 'I am a seller.');"
        "INSERT INTO users (name, account, password, description) VALUES ('Charlie', 'charlie789', 'pass3', 'I am a buyer.');"

        // Alice (id=1) 创建一个商店
        "INSERT INTO shops (name, invite_code, manager_id, creation_date, status, description) VALUES ('Alice Tech', 'ALICE01', 1, date('now', '-10 days'), 1, 'An awesome electronic store.');"
        "INSERT INTO shops (name, invite_code, manager_id, creation_date, status, description) VALUES ('Bookworm Haven', 'BOOKS02', 1, date('now', '-5 days'), 1, 'Your friendly neighborhood bookstore.');"

        // Bob (id=2) 加入 Alice 的第一个商店
        "INSERT INTO seller_shops (seller_id, shop_id) VALUES (2, 1);"

        // 添加几个类别
        "INSERT INTO categories (name) VALUES ('Electronics');"
        "INSERT INTO categories (name) VALUES ('Books');"
        "INSERT INTO categories (name) VALUES ('Computer');"
        "INSERT INTO categories (name) VALUES ('Fiction');"

        // Alice (id=1) 在她的第一个商店 (id=1) 发布商品
        "INSERT INTO items (name, seller_id, shop_id, price, publish_date, publish_time, description) VALUES ('Laptop', 1, 1, 1299.99, date('now', '-2 days'), time('now'), 'A powerful gaming laptop');"
        "INSERT INTO items (name, seller_id, shop_id, price, publish_date, publish_time, description) VALUES ('Sci-Fi Novel', 1, 2, 19.99, date('now', '-1 day'), time('now'), 'A journey to the stars.');"

        // Bob (id=2) 在 Alice 的第一个商店 (id=1) 发布商品
        "INSERT INTO items (name, seller_id, shop_id, price, publish_date, publish_time, description) VALUES ('Mouse', 2, 1, 25.50, date('now'), time('now'), 'A comfortable wireless mouse');"

        // 关联商品和类别
        "INSERT INTO item_categories (item_id, category_id) VALUES (1, 1);" // Laptop -> Electronics
        "INSERT INTO item_categories (item_id, category_id) VALUES (1, 3);" // Laptop -> Computer
        "INSERT INTO item_categories (item_id, category_id) VALUES (2, 2);" // Sci-Fi Novel -> Books
        "INSERT INTO item_categories (item_id, category_id) VALUES (2, 4);" // Sci-Fi Novel -> Fiction
        "INSERT INTO item_categories (item_id, category_id) VALUES (3, 1);" // Mouse -> Electronics

        // Charlie (id=3) 下一个订单，购买 Bob (id=2) 卖的鼠标 (item_id=3)
        "INSERT INTO orders (item_id, buyer_id, seller_id, shop_id, quantity, total_price, address, order_date, order_time, order_status) VALUES (3, 3, 2, 1, 2, 51.00, '123 Main St, Anytown', date('now'), time('now'), 0);";

    rc = sqlite3_exec(db, sql_insert_data, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error in initialize_case: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    }

    // 4. 关闭数据库连接
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
    // initialize_case();
}