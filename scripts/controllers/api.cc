#include "Controller.h"

const string IMAGE_FULL_PATH = "/home/amax/emall/data/images";

int idx_to_id(int idx, const string& type) {
    // TODO
    return idx;
}

int id_to_idx(int id, const string& type) {
    // TODO
    return id;
}

static size_t find_string_ic(std::string_view haystack, std::string_view needle) {
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
    );
    if (it == haystack.end()) return std::string_view::npos;
    return std::distance(haystack.begin(), it);
}

void save_image(const HttpRequestPtr &req, const string &type, int idx) {
    // 1. 获取 Boundary
    std::string contentType = req->getHeader("content-type");
    std::string boundaryTag = "boundary=";
    size_t boundaryPos = contentType.find(boundaryTag);

    if (boundaryPos == std::string::npos) {
        std::cout << "[error] 上传失败: 不是 multipart 请求" << std::endl;
        return;
    }

    std::string boundary = "--" + contentType.substr(boundaryPos + boundaryTag.length());
    // 清理 boundary 结尾可能的引号
    if (boundary.back() == '"') boundary.pop_back();
    if (boundary.size() > 2 && boundary[2] == '"') boundary.erase(2, 1);

    // 2. 获取原始 Body 数据
    std::string_view body = req->body();
    
    // 3. 寻找包含 name="photo" 的片段
    // 只要找到这个字段，就不管 filename 是什么了
    std::string_view nameTag = "name=\"photo\"";
    size_t namePos = body.find(nameTag);

    if (namePos == std::string_view::npos) {
        std::cout << "[error] 上传失败: 请求体中未找到 'photo' 字段" << std::endl;
        return;
    }

    // 4. 确定当前片段的范围
    // 往回找当前片段的开始 boundary
    size_t partStart = body.rfind(boundary, namePos);
    if (partStart == std::string_view::npos) {
        std::cout << "[error] 上传失败: 找不到片段开始" << std::endl;
        return;
    }

    // 往后找当前片段的结束 boundary
    size_t partEnd = body.find(boundary, namePos);
    if (partEnd == std::string_view::npos) {
        std::cout << "[error] 上传失败: 找不到片段结束" << std::endl;
        return;
    }

    // 5. 定位二进制数据的开始 (\r\n\r\n)
    // 从 namePos 往后找双换行，这是 Header 和 Body 的分界线
    size_t headerEnd = body.find("\r\n\r\n", namePos);
    if (headerEnd == std::string_view::npos || headerEnd > partEnd) {
        std::cout << "[error] 上传失败: 找不到数据起始符" << std::endl;
        return;
    }

    size_t dataStart = headerEnd + 4; // 跳过 \r\n\r\n
    size_t dataEnd = partEnd - 2;     // 减去 boundary 前面的 \r\n

    if (dataStart >= dataEnd) {
        std::cout << "[error] 上传失败: 文件内容为空" << std::endl;
        return;
    }

    // 6. 强制保存为 JPG
    std::string fullPath = IMAGE_FULL_PATH + "/" + type + "/" + to_string(idx) + ".jpg";

    try {
        std::ofstream outFile(fullPath, std::ios::binary);
        if (outFile.is_open()) {
            outFile.write(body.data() + dataStart, dataEnd - dataStart);
            outFile.close();
            // 打印文件大小，方便调试
            std::cout << "[info] 图片保存成功: " << fullPath 
                      << " (大小: " << (dataEnd - dataStart) / 1024 << " KB)" << std::endl;
        } else {
            std::cout << "[error] 无法打开文件路径: " << fullPath << std::endl;
        }
    } catch (const std::exception &e) {
        std::cout << "[error] 保存异常: " << e.what() << std::endl;
    }
}

string load_image(const string &type, int idx) {
    string file_path = IMAGE_FULL_PATH + "/" + type + "/" + to_string(idx) + ".jpg";
    if (std::filesystem::exists(file_path)) return file_path;
    string _none_;
    return _none_;
}

void delete_image(const string &type, int idx) {
    string file_path_str = IMAGE_FULL_PATH + "/" + type + "/" + to_string(idx) + ".jpg";
    try {
        std::filesystem::path path(file_path_str);
        if (std::filesystem::exists(path)) {
            // 执行删除
            if (std::filesystem::remove(path)) {
                std::cout << "[info]: 图片已删除: " << file_path_str << std::endl;
            } else {
                std::cout << "[error]: 图片删除失败 (可能被占用或权限不足): " << file_path_str << std::endl;
            }
        } else {
            // 文件不存在不算错误（可能用户本来就没上传过头像/商品图）
            std::cout << "[info]: 图片不存在，跳过删除: " << file_path_str << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[error]: 删除图片时发生异常: " << e.what() << std::endl;
    }
}

void initialize_file() {
    // 需要清空的三个子目录
    std::vector<std::string> subdirs = {"item", "shop", "user"};

    for (const auto& subdir : subdirs) {
        // 拼接完整路径
        std::string dir_str = IMAGE_FULL_PATH + "/" + subdir;
        std::filesystem::path dir_path(dir_str);

        try {
            // 1. 如果目录不存在，则创建它 (作为初始化的一部分)
            if (!std::filesystem::exists(dir_path)) {
                std::filesystem::create_directories(dir_path);
                std::cout << "[init]: 创建目录: " << dir_str << std::endl;
                continue;
            }

            // 2. 如果目录存在，遍历并删除其中所有内容
            int deleted_count = 0;
            for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
                // remove_all 可以删除文件，也可以删除子文件夹
                std::filesystem::remove_all(entry.path());
                deleted_count++;
            }

            if (deleted_count > 0) {
                std::cout << "[init]: 已清空 " << dir_str << " (删除了 " << deleted_count << " 个文件)" << std::endl;
            } else {
                std::cout << "[init]: 目录已为空: " << dir_str << std::endl;
            }

        } catch (const std::exception& e) {
            std::cout << "[error]: 初始化目录失败 " << dir_str << ": " << e.what() << std::endl;
        }
    }
}