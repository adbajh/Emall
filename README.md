## 项目概述

Emall 是一个类似闲鱼的线上交易平台，采用现代 Web 技术栈开发。平台支持用户注册、商品发布、订单管理、实时消息等核心功能，为买卖双方提供安全、便捷的交易环境。

## 技术栈

| 技术 | 版本/描述 |
|------|---------|
| **操作系统** | Ubuntu 22.04 |
| **Web 框架** | Drogon (C++ HTTP 框架) |
| **编程语言** | C++17 |
| **数据库** | SQLite3 |
| **构建工具** | CMake |
| **前端** | HTML/CSS/JavaScript |

## 项目结构

```
emall/
├── scripts/                    # 主项目目录
│   ├── CMakeLists.txt         # CMake 构建配置
│   ├── main.cc                # 应用入口
│   ├── config.json            # JSON 格式配置文件
│   ├── config.yaml            # YAML 格式配置文件
│   ├── controllers/           # HTTP 请求处理控制器
│   ├── filters/               # HTTP 过滤器 (如登录验证)
│   ├── models/                # 数据模型定义
│   ├── database/              # 数据库访问层
│   │   ├── database.h         # 数据库 API 头文件
│   │   ├── database.cc        # 数据库初始化和通用函数
│   │   ├── user.cc            # 用户相关数据库操作
│   │   ├── shop.cc            # 店铺相关数据库操作
│   │   ├── item.cc            # 商品相关数据库操作
│   │   ├── order.cc           # 订单相关数据库操作
│   │   ├── category.cc        # 分类相关数据库操作
│   │   └── message.cc         # 消息相关数据库操作
│   ├── public/                # 静态文件 (HTML/CSS/JS)
│   ├── views/                 # CSP 模板文件
│   ├── test/                  # 单元测试
│   ├── build/                 # 编译输出目录 (编译后生成)
│   └── plugins/               # Drogon 插件配置
├── data/                      # 数据存储，运行时创建
│   ├── database/
│   │   └── database.db        # SQLite3 数据库文件
│   └── images/                # 用户上传的图片
│       ├── item/              # 商品图片
│       ├── shop/              # 店铺图片
│       └── user/              # 用户头像
├── flaw_report/               # 代码缺陷分析报告
├── README.md                  # 项目说明文档
└── .gitignore                 # Git 忽略文件配置
```

## 功能模块

### 1. 用户管理 (User Management)
- ✅ 用户注册 (`user_register`)
- ✅ 用户登录 (`user_login`)
- ✅ 用户登出 (`user_logout`)
- ✅ 修改用户信息 (`user_modify_information`)
- ✅ 修改用户账号 (`user_modify_account`)
- ✅ 修改用户密码 (`user_modify_password`)
- ✅ 删除用户账号 (`user_delete_account`)
- ✅ 获取用户信息 (`get_user_information`)

### 2. 店铺管理 (Shop Management)
- ✅ 创建店铺 (`create_shop`)
- ✅ 修改店铺信息 (`modify_shop_information`)
- ✅ 删除店铺 (`delete_shop`)
- ✅ 加入店铺 (`attend_shop`)
- ✅ 离开店铺 (`leave_shop`)
- ✅ 获取店铺信息 (`get_shop_information`)

### 3. 商品管理 (Item Management)
- ✅ 发布商品 (`publish_item`)
- ✅ 修改商品信息 (`modify_item_information`)
- ✅ 删除商品 (`delete_item`)
- ✅ 获取商品信息 (`get_item_information`)
- ✅ 商品搜索 (`search_next_item_id`)

### 4. 订单管理 (Order Management)
- ✅ 创建订单 (`create_order`)
- ✅ 获取订单列表 (`get_orders`)
- ✅ 修改订单状态 (`edit_order_state`)

### 5. 消息系统 (Messaging System)
- ✅ 获取聊天联系人 (`get_chat_contacts`)
- ✅ 保存消息 (`save_message`)
- ✅ 获取消息历史 (支持分页)
- ✅ 标记消息已读

### 6. 分类管理 (Category Management)
- ✅ 获取所有商品分类 (`get_category_information`)
- ✅ 预设分类初始化

## 安装与编译

### 前置条件

```bash
# Ubuntu 22.04 系统依赖
sudo apt-get update
sudo apt-get install -y \
    git \
    cmake \
    build-essential \
    g++ \
    libsqlite3-dev \
    pkg-config \
    libssl-dev \
    uuid-dev

# 安装 Drogon 框架 (按照官方文档)
# https://github.com/drogonframework/drogon
```

### 编译步骤

```bash
# 1. 进入 scripts 目录
cd scripts

# 2. 创建 build 目录
mkdir -p build
cd build

# 3. 运行 CMake 配置
cmake ..

# 4. 编译项目
make

# 5. 返回项目主目录
cd ../..
```

### 运行应用

```bash
# 启动服务器
./scripts/build/emall

# 默认监听地址: 0.0.0.0:5555
# 服务端访问应用: http://localhost:5555/emall
# 客户端访问应用：http://ip:5555/emall (ip 替换成服务端的主机号)
```

## 在线演示

项目已部署在服务器上，可以直接访问。

**访问地址**: `http://114.212.20.69:5555/emall`

在浏览器中输入上述地址即可使用该平台。

## 数据库架构

### 核心表结构

#### 1. users (用户表)
```sql
CREATE TABLE users(
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    name         CHAR(10),
    account      CHAR(20) UNIQUE NOT NULL,
    password     CHAR(20) NOT NULL,
    token        CHAR(20),
    phone_number CHAR(13),
    email        CHAR(30),
    description  VARCHAR(100)
);
```

#### 2. shops (店铺表)
```sql
CREATE TABLE shops(
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    name          CHAR(10),
    invite_code   CHAR(10) UNIQUE NOT NULL,
    manager_id    INT NOT NULL,
    creation_date DATE,
    status        SMALLINT,
    description   VARCHAR(100),
    FOREIGN KEY(manager_id) REFERENCES users(id)
);
```

#### 3. items (商品表)
```sql
CREATE TABLE items(
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    name         CHAR(10),
    seller_id    INT NOT NULL,
    shop_id      INT NOT NULL,
    price        FLOAT,
    publish_date DATE,
    publish_time TIME,
    description  VARCHAR(100),
    FOREIGN KEY(seller_id) REFERENCES users(id),
    FOREIGN KEY(shop_id) REFERENCES shops(id)
);
```

#### 4. orders (订单表)
```sql
CREATE TABLE orders(
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    item_id      INT NOT NULL,
    buyer_id     INT NOT NULL,
    seller_id    INT NOT NULL,
    shop_id      INT NOT NULL,
    quantity     INT,
    total_price  FLOAT,
    address      VARCHAR(50),
    order_date   DATE,
    order_time   TIME,
    order_status TINYINT,
    FOREIGN KEY(item_id) REFERENCES items(id),
    FOREIGN KEY(buyer_id) REFERENCES users(id),
    FOREIGN KEY(seller_id) REFERENCES users(id),
    FOREIGN KEY(shop_id) REFERENCES shops(id)
);
```

#### 5. messages (消息表)
```sql
CREATE TABLE messages(
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    sender_id   INT NOT NULL,
    receiver_id INT NOT NULL,
    send_date   DATE,
    send_time   TIME,
    content     VARCHAR(100),
    is_read     TINYINT NOT NULL DEFAULT 0,
    FOREIGN KEY(sender_id) REFERENCES users(id),
    FOREIGN KEY(receiver_id) REFERENCES users(id)
);
```

## API 端点概览

### 认证相关
| 方法 | 端点 | 描述 |
|------|------|------|
| `POST` | `/emall/login` | 用户登录 |
| `POST` | `/emall/register` | 用户注册 |
| `POST` | `/emall/setting/logout` | 用户登出 |

### 用户管理
| 方法 | 端点 | 描述 |
|------|------|------|
| `GET` | `/emall/user?idx={}` | 获取用户信息 |
| `POST` | `/emall/setting/edit` | 修改用户信息 |
| `POST` | `/emall/setting/edit_account` | 修改账号 |
| `POST` | `/emall/setting/edit_password` | 修改密码 |
| `DELETE` | `/emall/setting/delete` | 删除账号 |

### 店铺管理
| 方法 | 端点 | 描述 |
|------|------|------|
| `GET` | `/emall/shop?idx={}` | 获取店铺信息 |
| `POST` | `/emall/manage/create` | 创建店铺 |
| `POST` | `/emall/manage/join` | 加入店铺 |
| `POST` | `/emall/manage/shop/{1}/edit` | 修改店铺信息 |
| `DELETE` | `/emall/manage/shop/{1}/dismiss` | 删除店铺 |

### 商品管理
| 方法 | 端点 | 描述 |
|------|------|------|
| `GET` | `/emall/item?idx={}` | 获取商品详情 |
| `POST` | `/emall/manage/shop/{}/publish` | 发布商品 |
| `POST` | `/emall/manage/shop/{1}/item/{2}/edit` | 修改商品 |
| `DELETE` | `/emall/manage/shop/{1}/item/{2}` | 删除商品 |

### 订单管理
| 方法 | 端点 | 描述 |
|------|------|------|
| `GET` | `/emall/orders` | 获取订单列表 |
| `POST` | `/emall/orders/create` | 创建订单 |
| `POST` | `/emall/orders/edit?idx={1}` | 修改订单状态 |

### 消息系统
| 方法 | 端点 | 描述 |
|------|------|------|
| `GET` | `/emall/messages` | 获取消息页面 |
| `GET` | `/emall/messages/get_history_liaison` | 获取联系人列表 |
| `POST` | `/emall/messages/send` | 发送消息 |
| `POST` | `/emall/messages/receive_latest` | 接收最新消息 |
| `POST` | `/emall/messages/receive_history` | 接收历史消息 |
| `POST` | `/emall/messages/mark_read` | 标记消息已读 |

## 配置文件

### config.yaml (推荐)
```yaml
# 监听地址和端口
listeners:
  - address: 0.0.0.0
    port: 5555
    https: false

# 日志配置
log:
  use_spdlog: false
  log_level: DEBUG
  
# HTTP 配置
app:
  document_root: /home/amax/emall/scripts/public
  upload_path: uploads
```

### config.json (备选)
JSON 格式的配置文件，与 YAML 格式功能相同。

## 开发注意事项

### 数据库 API 使用规范

1. **资源管理**
   - 始终使用 `sqlite3_prepare_v2` 预编译 SQL 语句
   - 操作完成后立即调用 `sqlite3_finalize(stmt)` 释放语句句柄
   - 操作完成后调用 `sqlite3_close(db)` 关闭数据库连接

2. **事务处理**
   - 复杂操作使用事务: `BEGIN TRANSACTION` / `COMMIT` / `ROLLBACK`
   - 确保失败时能够回滚所有修改

3. **参数绑定**
   - 使用参数绑定防止 SQL 注入: `sqlite3_bind_*`
   - 对于文本参数，使用 `SQLITE_TRANSIENT` 处理临时字符串

### 安全考量

> ⚠️ **已知安全问题** (见 flaw_report/Gemini)
> 
> - **CWE-79 (XSS 漏洞)**: 前端直接将用户输入渲染到 DOM
> - **CWE-772 (资源泄漏)**: 部分数据库操作未正确释放资源
> - **CWE-391 (未检查错误条件)**: 某些错误处理不完整
>
> 建议在生产环境前修复这些问题。

## 测试

### 运行单元测试

```bash
cd scripts/build
ctest --output-on-failure
```

### 测试文件位置

- test_main.cc - 测试入口

## 预设数据

应用初始化时会自动加载以下数据：

- **6 个测试用户**: Alice, Bob, Charlie, Diana, Eve, Frank
- **3 个店铺**: Alice科技, 书虫天堂, 戴安娜家居
- **7 个示例商品**: 游戏本, 智能手表, 历史巨著等
- **30+ 个商品分类**: 手机平板, 电脑整机, 图书等
- **多个示例订单和消息**

> 提示: 在 database.cc 的 `initialize_case()` 函数中修改预设数据

## 代码分析报告

项目已通过多种代码分析工具检查：

- **CppCheck** - C++ 静态分析
- **ESLint** - JavaScript 代码质量检查
- **Gemini** - AI 驱动的安全分析

详见 flaw_report 目录。

## 常见问题 (FAQ)

### Q: 如何初始化数据库？

A: 编辑 main.cc，取消注释以下行：
```cpp
initialize_db();     // 初始化表结构和预设数据
initialize_file();   // 初始化文件目录
```

### Q: 默认的服务器地址是什么？

A: `http://0.0.0.0:5555`，通过浏览器访问 `http://localhost:5555/emall`

### Q: 如何修改监听端口？

A: 编辑 main.cc 或配置文件中的端口设置

### Q: 数据库文件存储位置？

A: `data/database/database.db`

### Q: 用户上传的图片存储位置？

A: `data/images/{item|shop|user}/`

## 性能优化建议

1. **数据库优化**
   - 为频繁查询的列添加索引
   - 使用连接池管理数据库连接
   - 考虑从 SQLite3 迁移到 PostgreSQL/MySQL

2. **缓存策略**
   - 缓存热门商品列表
   - 缓存用户信息

3. **前端优化**
   - 使用 CDN 加速静态资源
   - 启用 GZIP 压缩 (已在配置中开启)
   - 实现图片懒加载

## 许可证

[暂无]

## 贡献指南

欢迎提交 Issue 和 Pull Request！

## 联系方式

- **项目维护者**: [陆博文]
- **邮箱**: [3560967301@qq.com]

---

**最后更新**: 2025 年 11 月 13 日  
**项目版本**: 1.0.0