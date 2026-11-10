#pragma once
#include <filesystem>
#include <system_error>
#include <vector>

namespace inspection {
struct Entry {
    std::filesystem::path path;
    std::filesystem::file_type type = std::filesystem::file_type::none;
    std::error_code error;
};

struct Report {
    std::vector<Entry> entries;
    std::error_code traversal_error;
    std::size_t workers = 0;
};

// Uses error_code overloads for ordinary filesystem failures.
// Allocation and thread-creation failures can still throw.
Report scan(const std::filesystem::path& root, unsigned requested_workers);
} // namespace inspection
