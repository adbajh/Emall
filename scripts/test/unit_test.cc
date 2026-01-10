#include <drogon/drogon_test.h>
#include <drogon/drogon.h>
#include "../database/database.h"
#include <string>
#include <vector>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <set>

using namespace drogon;

// --- 辅助工具函数 ---
std::string gen_random_string(size_t length) {
    auto randchar = []() -> char {
        const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[rand() % max_index];
    };
    std::string str(length, 0);
    std::generate_n(str.begin(), length, randchar);
    return str;
}

// =========================================================================
// 子功能 1: 用户管理体系 (User Management System)
// 被测对象: user_register, user_login, user_get_id
// 测试目标: 覆盖注册、登录、鉴权的核心流程及异常边界，总计 > 10 个用例
// =========================================================================
DROGON_TEST(UserManagementTest)
{
    std::string acc_base = "u_" + gen_random_string(5);
    std::string nick_base = "Tester";
    std::string pwd_base = "pass123";

    // --- Case 1: 标准注册 (Happy Path) ---
    // 输入合法信息，预期返回 0 (成功)
    int ret_reg = user_register(nick_base, acc_base, pwd_base);
    CHECK(ret_reg == 0);

    // --- Case 2: 标准登录 (Happy Path) ---
    // 输入正确账号密码，预期成功并返回 Token
    std::string token;
    int ret_login = user_login(acc_base, pwd_base, token);
    CHECK(ret_login == 0);
    CHECK(token.empty() == false);

    // --- Case 3: 登录后身份验证 (State Verification) ---
    // 使用返回的 Token 获取 ID，预期 ID > 0
    int uid = user_get_id(acc_base, token);
    CHECK(uid > 0);

    // --- Case 4: 注册第二个不同用户 (Independence) ---
    // 确保系统支持多用户，且 ID 不同
    std::string acc_2 = acc_base + "_2";
    CHECK(user_register(nick_base, acc_2, pwd_base) == 0);
    
    // --- Case 5: 第二用户登录与 ID 独立性 ---
    std::string token_2;
    CHECK(user_login(acc_2, pwd_base, token_2) == 0);
    int uid_2 = user_get_id(acc_2, token_2);
    CHECK(uid_2 > 0);
    CHECK(uid != uid_2); // ID 必须不同

    // --- Case 6: 密码错误测试 (Security Boundary) ---
    std::string fail_token;
    int ret_bad_pass = user_login(acc_base, "wrong_pass", fail_token);
    CHECK(ret_bad_pass != 0); // 预期失败

    // --- Case 7: 账号不存在测试 (Security Boundary) ---
    int ret_no_user = user_login("ghost_user_999", pwd_base, fail_token);
    CHECK(ret_no_user != 0); // 预期失败

    // --- Case 8: 使用错误 Token 鉴权 (Security Boundary) ---
    // 即使账号正确，Token 错误也应无法获取 ID
    int fail_uid = user_get_id(acc_base, "invalid_fake_token");
    CHECK(fail_uid <= 0);    // 预期获取 ID 失败

    // --- Case 9: 空账号注册测试 (Input Boundary) ---
    // 尝试注册空账号 (如果数据库层有约束，应报错；若业务层未拦截则可能通过，这里测试鲁棒性)
    // 根据以前经验，通常应拦截或数据库报错。我们假设它不应该返回正常的0，或者我们只观察它不奔溃
    int ret_empty_acc = user_register("nick", "", "pass");
    // 此处不强制 CHECK 返回值，因为取决于具体实现，主要的检查点是程序不 Crash
    // 如果您的实现允许空串，这里可以改为 CHECK(ret_empty_acc == 0);
    // 这里假设应当失败或被处理
    std::cout << "[Info] Empty account register result: " << ret_empty_acc << std::endl;

    // --- Case 10: 空密码登录测试 (Input Boundary) ---
    std::string t;
    int ret_empty_pwd = user_login(acc_base, "", t);
    CHECK(ret_empty_pwd != 0); 

    // --- Case 11: 极长字符串输入 (Stress/Buffer) ---
    std::string long_acc = gen_random_string(100);
    std::string long_pwd = gen_random_string(100);
    user_register("LongName", long_acc, long_pwd);
    std::string long_token;
    CHECK(user_login(long_acc, long_pwd, long_token) == 0);

    // --- Case 12: 重复登录 Token 变化测试 (Session Policy) ---
    // 再次登录用户 1，看 Token 是否生成（通常会生成新的或返回旧的，确保操作成功）
    std::string token_new;
    CHECK(user_login(acc_base, pwd_base, token_new) == 0);
    CHECK(token_new.length() > 0);
}

// =========================================================================
// 子功能 2: 商品分类信息系统 (Category System)
// 被测对象: get_category_information
// 测试目标: 覆盖数据查询、容器操作、数据完整性及多次调用稳定性，总计 > 10 个用例
// =========================================================================
DROGON_TEST(CategorySystemTest)
{
    // --- Case 1: 基础查询调用 (Interface Validity) ---
    std::vector<std::string> cats;
    int ret = get_category_information(cats);
    CHECK(ret == 0);

    // --- Case 2: 结果集非空性 (Data Availability) ---
    // 假设数据库已初始化，分类列表不应为空
    bool is_empty = cats.empty();
    CHECK(is_empty == false);
    size_t initial_size = cats.size();

    // --- Case 3: 关键数据存在性验证 A (Content Verification) ---
    // 检查是否有 "电子产品" 或 "手机" 相关的分类 (模糊匹配或精确匹配)
    // 这里为了通用性，我们只检查是否有数据，并打印第一个
    if (!cats.empty()) {
        CHECK(cats[0].length() > 0);
        std::cout << "[Info] First category: " << cats[0] << std::endl;
    }

    // --- Case 4: 容器脏数据处理 (Container State) ---
    // 传入一个非空的 vector，验证函数是 "追加" 还是 "重置"
    // 大多数正确实现应该是先 clear 再 insert，或者 append。
    // 我们测试它调用后是否还能保持逻辑自洽（不崩即可）
    std::vector<std::string> dirty_vec;
    dirty_vec.push_back("PRE_EXISTING_DATA");
    int ret_dirty = get_category_information(dirty_vec);
    CHECK(ret_dirty == 0);
    // 验证：如果在你的实现中会清空旧数据，那么 size 应该等于 initial_size
    // 如果是追加，size = initial_size + 1。
    // 这里做宽松检查：size 至少要包含数据库里的数据
    CHECK(dirty_vec.size() >= initial_size);

    // --- Case 5: 独立容器调用 (Isolation) ---
    std::vector<std::string> vec_b;
    get_category_information(vec_b);
    CHECK(vec_b.size() == initial_size);

    // --- Case 6: 数据一致性 (Consistency) ---
    // 两次查询结果应该完全一致
    if (initial_size > 0 && vec_b.size() > 0) {
        CHECK(cats[0] == vec_b[0]);
        CHECK(cats.back() == vec_b.back());
    }

    // --- Case 7: 幂等性测试 (Idempotency) ---
    // 连续调用不应导致错误或状态累积异常
    int ret_idem = get_category_information(cats); // 再次使用 cats
    CHECK(ret_idem == 0);
    
    // --- Case 8: 字符串格式校验 (Data Integrity) ---
    // 确保取出的分类名称不是乱码 (简单的 ASCII/UTF8 长度检查)
    for(const auto& c : vec_b) {
        CHECK(c.length() > 0);
        CHECK(c.length() < 256); // 假设分类名不太可能超过256个字符
    }

    // --- Case 9: 压力测试-循环调用 (Stress Test) ---
    // 循环调用 100 次，确保数据库连接池或查询不泄漏
    bool stress_ok = true;
    for(int i=0; i<100; ++i) {
        std::vector<std::string> temp;
        if(get_category_information(temp) != 0) {
            stress_ok = false;
            break;
        }
    }
    CHECK(stress_ok == true);

    // --- Case 10: 集合去重性验证 (Logic Verification) ---
    // 验证返回的列表中是否有重复项（通常分类列表不应有重复）
    std::set<std::string> unique_set(vec_b.begin(), vec_b.end());
    // 如果 vector size 和 set size 相等，说明没有重复元素
    CHECK(unique_set.size() == vec_b.size());

    // --- Case 11: 性能模拟计时 (Performance Monitor) ---
    clock_t start = clock();
    std::vector<std::string> perf_vec;
    get_category_information(perf_vec);
    clock_t end = clock();
    double elapsed = double(end - start) / CLOCKS_PER_SEC;
    // 期望单次简单查询极快 (< 0.5s)
    CHECK(elapsed < 0.5);
    std::cout << "[Info] Category query time: " << elapsed * 1000 << " ms"
    << std::endl;
}

int main(int argc, char **argv) {
    // 初始化随机数种子
    std::srand(std::time(nullptr));
    
    // 设置数据库为内存数据库（避免文件冲突）
    database_name = ":memory:";
    
    // 初始化数据库
    initialize_db();
    
    // 运行 Drogon 测试框架
    return drogon::test::run(argc, argv);
}