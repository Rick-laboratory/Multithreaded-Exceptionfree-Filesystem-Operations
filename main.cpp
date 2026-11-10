#include "scan.hpp"

#include <iostream>
#include <string>
#include <thread>

namespace {
std::string utf8_path(const std::filesystem::path& path) {
    const auto bytes = path.u8string();
    // C++17 returns string; C++20 returns u8string. Preserve the encoded bytes.
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

template <typename Character>
int run(int argc, Character* argv[]) {
    namespace fs = std::filesystem;
    try {
        if (argc == 2 && (fs::path(argv[1]) == "--help" || fs::path(argv[1]) == "-h")) {
            std::cout << "Usage: filesystem-inspect [directory]\n"
                         "List regular files under a directory; default: current directory.\n"
                         "Paths are printed as UTF-8. No files are modified or opened for content.\n";
            return 0;
        }
        if (argc > 2) {
            std::cerr << "Usage: filesystem-inspect [directory]\n";
            return 2;
        }

        const fs::path root = argc == 2 ? fs::path(argv[1]) : fs::current_path();
        const inspection::Report report = inspection::scan(root, std::thread::hardware_concurrency());
        std::size_t regular_files = 0;
        std::size_t failures = 0;
        for (const auto& entry : report.entries) {
            if (entry.error) {
                ++failures;
                std::cerr << utf8_path(entry.path) << ": " << entry.error.message() << '\n';
            } else if (entry.type == fs::file_type::regular) {
                ++regular_files;
                std::cout << utf8_path(entry.path) << '\n';
            }
        }
        if (report.traversal_error) {
            ++failures;
            std::cerr << "Traversal: " << report.traversal_error.message() << '\n';
        }
        std::cerr << "Inspected " << report.entries.size() << " entries with " << report.workers
                  << " workers; " << regular_files << " regular files; " << failures << " errors.\n";
        return failures == 0 ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
} // namespace

#ifdef _WIN32
int wmain(int argc, wchar_t* argv[]) { return run(argc, argv); }
#else
int main(int argc, char* argv[]) { return run(argc, argv); }
#endif
