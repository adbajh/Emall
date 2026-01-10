cmake_minimum_required(VERSION 3.15)
project(emall_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ✅ 查找依赖
find_package(Drogon CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# ✅ 启用测试
enable_testing()

# ✅ 单元测试可执行文件
add_executable(unit_test 
    unit_test.cc
    ../database/user.cc
    ../database/shop.cc
    ../database/item.cc
    ../database/database.cc
    ../database/category.cc
    ../database/order.cc
    ../database/message.cc
)

target_link_libraries(unit_test 
    PRIVATE 
    Drogon::Drogon
    ${SQLITE3_LIBRARIES}
    pthread
)

target_include_directories(unit_test 
    PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${SQLITE3_INCLUDE_DIRS}
)

# ✅ 注册测试
add_test(NAME UnitTests COMMAND unit_test)