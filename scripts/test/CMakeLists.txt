cmake_minimum_required(VERSION 3.15)  # ✅ 升级最低版本要求

project(emall_fuzz CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ✅ 仅在找到 AFL++ 时设置编译器
if(EXISTS "/usr/bin/afl-clang-fast++")
    message(STATUS "Found AFL++ compiler, using for fuzzing")
    set(CMAKE_CXX_COMPILER "/usr/bin/afl-clang-fast++")
    set(CMAKE_C_COMPILER "/usr/bin/afl-clang-fast")
    
    # AFL++ 特定编译选项
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -funroll-loops")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/usr/lib/x86_64-linux-gnu")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath,/usr/lib/x86_64-linux-gnu")
else()
    message(WARNING "AFL++ not found, building with default compiler")
    # 使用默认编译器，但仍然添加 ASAN
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
endif()

# ✅ 清理环境变量（仅在使用 AFL++ 时）
if(CMAKE_CXX_COMPILER MATCHES "afl-clang")
    set(ENV{LD_LIBRARY_PATH} "")
    set(ENV{LIBRARY_PATH} "")
    set(ENV{CPATH} "")
    set(ENV{C_INCLUDE_PATH} "")
    set(ENV{CPLUS_INCLUDE_PATH} "")
endif()

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