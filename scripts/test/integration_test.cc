#include <drogon/drogon_test.h>
#include <drogon/drogon.h>
#include "../database/database.h"
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <ctime>

using namespace drogon;

// --- 辅助函数 ---
namespace {
    // 生成随机字符串
    std::string gen_rand_str(size_t length) {
        auto randchar = []() -> char {
            const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
            const size_t max_index = (sizeof(charset) - 1);
            return charset[rand() % max_index];
        };
        std::string str(length, 0);
        std::generate_n(str.begin(), length, randchar);
        return str;
    }

    // 获取当前日期字符串 (YYYY-MM-DD)
    std::string get_date_str() {
        time_t now = time(0);
        struct tm tstruct;
        char buf[80];
        tstruct = *localtime(&now);
        strftime(buf, sizeof(buf), "%Y-%m-%d", &tstruct);
        return std::string(buf);
    }

    // 获取当前时间字符串 (HH:MM:SS)
    std::string get_time_str() {
        time_t now = time(0);
        struct tm tstruct;
        char buf[80];
        tstruct = *localtime(&now);
        strftime(buf, sizeof(buf), "%X", &tstruct);
        return std::string(buf);
    }

    // 辅助函数: 注册并登录用户,返回用户ID
    int setup_user(const std::string& prefix, std::string& out_acc, std::string& out_pwd, std::string& out_token) {
        out_acc = prefix + "_" + gen_rand_str(5);
        out_pwd = "pwd_" + gen_rand_str(5);
        std::string nick = prefix + "_Nickname";
        
        // 1. 注册
        if (user_register(nick, out_acc, out_pwd) != 0) {
            std::cout << "[Error] Failed to register user: " << out_acc << std::endl;
            return -1;
        }
        
        // 2. 登录获取token
        if (user_login(out_acc, out_pwd, out_token) != 0) {
            std::cout << "[Error] Failed to login user: " << out_acc << std::endl;
            return -2;
        }
        
        // 3. 获取用户ID
        int user_id = user_get_id(out_acc, out_token);
        if (user_id > 0) {
            std::cout << "[Info] Setup user '" << prefix << "' with ID: " << user_id << std::endl;
        }
        return user_id;
    }
}

// =========================================================================
// 集成测试组 1: 电商交易完整链路测试 (自顶向下)
// 流程: 卖家注册 -> 创建店铺 -> 发布商品 -> 买家注册 -> 下单购买 -> 验证订单
// 测试目标: 验证 User、Shop、Item、Order 四个模块的数据流转和外键关联
// =========================================================================
DROGON_TEST(TransactionIntegrationTest)
{
    std::cout << "\n========== Transaction Integration Test ==========" << std::endl;

    // --- Step 1: 准备卖家账户 ---
    std::string seller_acc, seller_pwd, seller_token;
    int seller_id = setup_user("Seller", seller_acc, seller_pwd, seller_token);
    CHECK(seller_id > 0);

    // --- Step 2: 卖家创建店铺 ---
    // 根据 database.h 中 ShopInfo 的定义构建
    ShopInfo shop_info;
    shop_info.name = "TestShop_" + gen_rand_str(4);
    shop_info.invite_code = "INTEG" + gen_rand_str(3);  // 生成唯一邀请码
    shop_info.description = "Integration Test Shop Description";
    
    int shop_id = create_shop(seller_id, shop_info);
    CHECK(shop_id > 0);  // create_shop 返回新店铺ID
    std::cout << "[Info] Created shop with ID: " << shop_id << std::endl;

    // --- Step 3: 卖家在店铺中发布商品 ---
    // 根据 database.h 中 Item 结构体定义
    Item item_info;
    item_info.name = "TestItem_" + gen_rand_str(4);
    item_info.price = 99.99;
    item_info.description = "Integration Test Item Description";
    item_info.types.push_back("电子产品");  // 添加商品分类
    
    int item_id = publish_item(seller_id, shop_id, item_info);
    CHECK(item_id > 0);  // publish_item 返回新商品ID
    std::cout << "[Info] Published item with ID: " << item_id << std::endl;

    // --- Step 4: 准备买家账户 ---
    std::string buyer_acc, buyer_pwd, buyer_token;
    int buyer_id = setup_user("Buyer", buyer_acc, buyer_pwd, buyer_token);
    CHECK(buyer_id > 0);
    CHECK(buyer_id != seller_id);  // 确保买卖家是不同用户

    // --- Step 5: 买家下单购买商品 ---
    int quantity = 2;
    std::string address = "Integration Test Address 123";
    bool is_paid = true;
    
    int order_result = create_order(buyer_id, item_id, quantity, address, is_paid);
    CHECK(order_result == 0);  // create_order 返回0表示成功
    std::cout << "[Info] Order created successfully" << std::endl;

    // --- Step 6: 验证订单数据完整性 ---
    std::vector<Order> buyer_orders;
    int get_result = get_orders(buyer_id, true, -1, buyer_orders);  // true表示买家视角,-1表示所有状态
    
    CHECK(get_result == 0);
    CHECK(buyer_orders.empty() == false);  // 应该至少有一个订单
    
    if (!buyer_orders.empty()) {
        // 验证订单中的关键字段
        bool found_order = false;
        for (const auto& order : buyer_orders) {
            if (order.item_id == item_id && order.buyer_id == buyer_id) {
                found_order = true;
                CHECK(order.quantity == quantity);
                CHECK(order.seller_id == seller_id);
                CHECK(order.shop_id == shop_id);
                std::cout << "[Info] Order verification passed: item_id=" << order.item_id 
                          << ", quantity=" << order.quantity << std::endl;
                break;
            }
        }
        CHECK(found_order == true);
    }

    std::cout << "========== Transaction Test PASSED ==========" << std::endl;
}

// =========================================================================
// 集成测试组 2: 用户社交消息链路测试 (自底向上)
// 流程: 用户A注册 -> 用户B注册 -> A发送消息给B -> 验证消息存储
// 测试目标: 验证 User 和 Message 模块间的交互和外键约束
// =========================================================================
DROGON_TEST(MessageIntegrationTest)
{
    std::cout << "\n========== Message Integration Test ==========" << std::endl;

    // --- Step 1: 准备用户A (发送方) ---
    std::string user_a_acc, user_a_pwd, user_a_token;
    int user_a_id = setup_user("Alice", user_a_acc, user_a_pwd, user_a_token);
    CHECK(user_a_id > 0);

    // --- Step 2: 准备用户B (接收方) ---
    std::string user_b_acc, user_b_pwd, user_b_token;
    int user_b_id = setup_user("Bob", user_b_acc, user_b_pwd, user_b_token);
    CHECK(user_b_id > 0);
    CHECK(user_a_id != user_b_id);

    // --- Step 3: 用户A发送消息给用户B ---
    std::string message_content = "Integration Test Message: " + gen_rand_str(8);
    std::string send_date = get_date_str();
    std::string send_time = get_time_str();
    
    int send_result = save_message(user_a_id, user_b_id, message_content, send_date, send_time);
    CHECK(send_result == 0);  // 0表示保存成功
    std::cout << "[Info] Message sent from user " << user_a_id << " to user " << user_b_id << std::endl;

    // --- Step 4: 验证消息的数据库外键约束 ---
    // 由于 save_message 返回0,说明:
    // 1. sender_id (user_a_id) 外键约束通过
    // 2. receiver_id (user_b_id) 外键约束通过
    // 3. 消息成功写入数据库
    
    // 进一步验证: 尝试用无效用户ID发送消息 (应该失败)
    int invalid_user_id = 999999;
    int fail_result = save_message(invalid_user_id, user_b_id, "Should Fail", send_date, send_time);
    CHECK(fail_result != 0);  // 应该因为外键约束失败
    std::cout << "[Info] Invalid sender test passed (failed as expected)" << std::endl;

    std::cout << "========== Message Test PASSED ==========" << std::endl;
}

// =========================================================================
// 集成测试组 3: 店铺管理与权限链路测试
// 流程: 经理创建店铺 -> 销售员加入店铺 -> 销售员发布商品 -> 验证权限
// 测试目标: 验证 Shop 和 Item 的权限控制逻辑
// =========================================================================
DROGON_TEST(ShopManagementIntegrationTest)
{
    std::cout << "\n========== Shop Management Integration Test ==========" << std::endl;

    // --- Step 1: 经理创建店铺 ---
    std::string manager_acc, manager_pwd, manager_token;
    int manager_id = setup_user("Manager", manager_acc, manager_pwd, manager_token);
    CHECK(manager_id > 0);

    ShopInfo shop;
    shop.name = "MgmtShop_" + gen_rand_str(3);
    shop.invite_code = "MGMT" + gen_rand_str(4);
    shop.description = "Management Test Shop";
    
    int shop_id = create_shop(manager_id, shop);
    CHECK(shop_id > 0);

    // --- Step 2: 销售员加入店铺 ---
    std::string seller_acc, seller_pwd, seller_token;
    int seller_id = setup_user("Seller", seller_acc, seller_pwd, seller_token);
    CHECK(seller_id > 0);

    int join_result = attend_shop(seller_id, shop.invite_code);
    CHECK(join_result == 0);  // 0表示加入成功
    std::cout << "[Info] Seller joined shop successfully" << std::endl;

    // --- Step 3: 销售员在店铺中发布商品 ---
    Item seller_item;
    seller_item.name = "SellerItem_" + gen_rand_str(3);
    seller_item.price = 49.99;
    seller_item.description = "Item published by seller";
    seller_item.types.push_back("图书");

    int seller_item_id = publish_item(seller_id, shop_id, seller_item);
    CHECK(seller_item_id > 0);  // 销售员应该有权限发布
    std::cout << "[Info] Seller published item with ID: " << seller_item_id << std::endl;

    // --- Step 4: 验证未加入店铺的用户无法发布商品 ---
    std::string outsider_acc, outsider_pwd, outsider_token;
    int outsider_id = setup_user("Outsider", outsider_acc, outsider_pwd, outsider_token);
    CHECK(outsider_id > 0);

    Item outsider_item;
    outsider_item.name = "ShouldFail";
    outsider_item.price = 1.0;
    outsider_item.description = "Unauthorized";
    outsider_item.types.push_back("测试");

    int unauthorized_result = publish_item(outsider_id, shop_id, outsider_item);
    CHECK(unauthorized_result == -1);  // -1表示无权限,发布失败
    std::cout << "[Info] Unauthorized publish test passed (failed as expected)" << std::endl;

    std::cout << "========== Shop Management Test PASSED ==========" << std::endl;
}