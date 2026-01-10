cmake_minimum_required(VERSION 3.15)  # ✅ 升级最低版本要求
project(emall CXX)

include(CheckIncludeFileCXX)

check_include_file_cxx(any HAS_ANY)
check_include_file_cxx(string_view HAS_STRING_VIEW)
check_include_file_cxx(coroutine HAS_COROUTINE)
if (NOT "${CMAKE_CXX_STANDARD}" STREQUAL "")
    # Do nothing
elseif (HAS_ANY AND HAS_STRING_VIEW AND HAS_COROUTINE)
    set(CMAKE_CXX_STANDARD 20)
elseif (HAS_ANY AND HAS_STRING_VIEW)
    set(CMAKE_CXX_STANDARD 17)
else ()
    set(CMAKE_CXX_STANDARD 14)
endif ()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${PROJECT_NAME} main.cc)

# ✅ 先查找 Drogon（它会自动查找 SQLite3）
find_package(Drogon CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon)

# ✅ 移除重复的 find_package(SQLite3)，改为直接链接
# Drogon 已经找到了 SQLite3，我们可以直接使用
if(TARGET SQLite::SQLite3)
    message(STATUS "Using SQLite3 from Drogon")
    target_link_libraries(${PROJECT_NAME} PRIVATE SQLite::SQLite3)
elseif(TARGET SQLite3_lib)
    message(STATUS "Using SQLite3_lib from Drogon")
    target_link_libraries(${PROJECT_NAME} PRIVATE SQLite3_lib)
else()
    # ✅ 仅在 Drogon 没有找到 SQLite3 时才查找
    find_package(SQLite3 REQUIRED)
    if(SQLITE3_FOUND)
        message(STATUS "Found SQLite3. Linking using legacy variables: ${SQLITE3_LIBRARIES}")
        target_include_directories(${PROJECT_NAME} PRIVATE ${SQLITE3_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${SQLITE3_LIBRARIES})
    else()
        message(FATAL_ERROR "SQLite3 was required but not found.")
    endif()
endif()

if (CMAKE_CXX_STANDARD LESS 17)
    message(FATAL_ERROR "c++17 or higher is required")
elseif (CMAKE_CXX_STANDARD LESS 20)
    message(STATUS "use c++17")
else ()
    message(STATUS "use c++20")
endif ()

aux_source_directory(controllers CTL_SRC)
aux_source_directory(filters FILTER_SRC)
aux_source_directory(plugins PLUGIN_SRC)
aux_source_directory(models MODEL_SRC)
aux_source_directory(database DATABASE_SRC)

drogon_create_views(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/views
                    ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(${PROJECT_NAME}
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                                   ${CMAKE_CURRENT_SOURCE_DIR}/models)
target_sources(${PROJECT_NAME}
               PRIVATE
               ${SRC_DIR}
               ${CTL_SRC}
               ${FILTER_SRC}
               ${PLUGIN_SRC}
               ${MODEL_SRC}
               ${DATABASE_SRC})

# ✅ 仅在本地开发时添加 test 子目录
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
    # 检查是否在 CI 环境中
    if(NOT DEFINED ENV{CI})
        add_subdirectory(test)
    else()
        message(STATUS "Skipping test subdirectory in CI environment")
    endif()
endif()