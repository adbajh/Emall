#include "../database/database.h"
#include <cstring>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

// ✅ 全局初始化标志
static bool g_db_initialized = false;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    // 至少需要 1 字节来决定测试哪个函数
    if (size < 1) return 0;
    
    // ✅ 只初始化一次数据库
    if (!g_db_initialized) {
        mkdir("/home/amax/emall/data/tmp", 0755);
        database_name = "/home/amax/emall/data/tmp/fuzz_test.db";
        initialize_db();
        g_db_initialized = true;
    }
    
    // 第一个字节决定测试哪个 API
    uint8_t function_selector = data[0];
    data++;
    size--;
    
    switch (function_selector % 8) {
        case 0: { // 测试用户注册
            if (size < 6) return 0;
            size_t name_len = std::min(size/3, (size_t)20);
            size_t account_len = std::min(size/3, (size_t)20);
            size_t pass_len = size - name_len - account_len;
            
            std::string username((char*)data, name_len);
            std::string account((char*)data + name_len, account_len);
            std::string password((char*)data + name_len + account_len, pass_len);
            user_register(username, account, password);
            break;
        }
        case 1: { // 测试用户登录
            if (size < 4) return 0;
            std::string account((char*)data, size/2);
            std::string password((char*)data + size/2, size - size/2);
            std::string token;
            user_login(account, password, token);
            break;
        }
        case 2: { // 测试店铺创建
            if (size < 9) return 0;
            int user_id = *(int32_t*)data;
            ShopInfo shop;
            size_t name_len = std::min((size-4)/3, (size_t)20);
            size_t code_len = std::min((size-4)/3, (size_t)10);
            shop.name = std::string((char*)data + 4, name_len);
            shop.invite_code = std::string((char*)data + 4 + name_len, code_len);
            shop.description = std::string((char*)data + 4 + name_len + code_len, 
                                          size - 4 - name_len - code_len);
            create_shop(user_id, shop);
            break;
        }
        case 3: { // 测试商品发布
            if (size < 12) return 0;
            int user_id = *(int32_t*)data;
            int shop_id = *(int32_t*)(data + 4);
            Item item;
            size_t name_len = std::min((size-8)/2, (size_t)20);
            item.name = std::string((char*)data + 8, name_len);
            item.description = std::string((char*)data + 8 + name_len, size - 8 - name_len);
            item.price = 99.99;
            publish_item(user_id, shop_id, item);
            break;
        }
        case 4: { // 测试订单创建
            if (size < 12) return 0;
            int buyer_id = *(int32_t*)data;
            int item_id = *(int32_t*)(data + 4);
            int quantity = *(int32_t*)(data + 8);
            std::string address((char*)data + 12, size - 12);
            create_order(buyer_id, item_id, quantity, address, true);
            break;
        }
        case 5: { // 测试消息发送
            if (size < 8) return 0;
            int sender = *(int32_t*)data;
            int receiver = *(int32_t*)(data + 4);
            std::string content((char*)data + 8, size - 8);
            save_message(sender, receiver, content, "2024-01-01", "12:00:00");
            break;
        }
        case 6: { // 测试店铺加入
            if (size < 4) return 0;
            int user_id = *(int32_t*)data;
            std::string code((char*)data + 4, size - 4);
            attend_shop(user_id, code);
            break;
        }
        case 7: { // 测试商品搜索
            if (size < 4) return 0;
            int cur_id = *(int32_t*)data;
            std::map<std::string, std::string> cond;
            if (size > 4) {
                std::string query((char*)data + 4, size - 4);
                cond["name"] = query;
            }
            search_next_item_id(cur_id, cond);
            break;
        }
    }
    
    return 0;
}

// ✅ 简化的 main 函数 - 移除所有 AFL++ 持久化模式相关代码
int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }
    
    // 打开输入文件
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }
    
    // 读取文件内容
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size < 0) {
        perror("ftell");
        fclose(f);
        return 1;
    }
    fseek(f, 0, SEEK_SET);
    
    uint8_t *data = (uint8_t*)malloc(size);
    if (!data) {
        fprintf(stderr, "malloc failed\n");
        fclose(f);
        return 1;
    }
    
    size_t read_size = fread(data, 1, size, f);
    fclose(f);
    
    // ✅ 只执行一次测试（标准 fork 模式）
    LLVMFuzzerTestOneInput(data, read_size);
    
    free(data);
    return 0;
}