cmake_minimum_required(VERSION 3.5)

# ==================== 强制隔离 Conda 环境 ====================
set(ENV{LD_LIBRARY_PATH} "")
set(ENV{LIBRARY_PATH} "")
set(ENV{CPATH} "")
set(ENV{C_INCLUDE_PATH} "")
set(ENV{CPLUS_INCLUDE_PATH} "")

# 使用 AFL++ 编译器
set(CMAKE_CXX_COMPILER "/usr/bin/afl-clang-fast++")
set(CMAKE_C_COMPILER "/usr/bin/afl-clang-fast")

set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_C_COMPILER_WORKS 1)

project(emall_fuzz CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==================== 编译和链接选项 ====================
# 移除 -nostdinc++，让编译器自动处理标准库
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -funroll-loops")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")

# 明确指定使用系统库路径（链接阶段）
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/usr/lib/x86_64-linux-gnu")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,/usr/lib/x86_64-linux-gnu")

# 添加 <random> 头文件需要的链接库
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

set(FUZZ_SOURCES
    fuzz_target.cc
    ../database/user.cc
    ../database/shop.cc
    ../database/item.cc
    ../database/database.cc
    ../database/category.cc
    ../database/order.cc
    ../database/message.cc
)

add_executable(emall_fuzz ${FUZZ_SOURCES})
target_include_directories(emall_fuzz PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

target_link_libraries(emall_fuzz 
    PRIVATE 
    ${SQLITE3_LIBRARIES}
    pthread
)

target_include_directories(emall_fuzz PRIVATE ${SQLITE3_INCLUDE_DIRS})