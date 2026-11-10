#include "scan.hpp"

#include <algorithm>
#include <atomic>
#include <future>

namespace inspection {
Report scan(const std::filesystem::path& root, unsigned requested_workers) {
    namespace fs = std::filesystem;
    Report report;
    std::error_code error;
    fs::recursive_directory_iterator iterator(root, fs::directory_options::skip_permission_denied,
                                              error);
    if (error) {
        report.traversal_error = error;
        return report;
    }

    const fs::recursive_directory_iterator end;
    while (iterator != end) {
        report.entries.push_back({iterator->path(), fs::file_type::none, {}});
        iterator.increment(error);
        if (error) {
            report.traversal_error = error;
            break;
        }
    }
    if (report.entries.empty()) return report;

    // Zero is a valid hardware_concurrency result; always retain a fallback worker.
    report.workers = std::min<std::size_t>(
        std::min<std::size_t>(std::max(1U, requested_workers), 32), report.entries.size());
    std::atomic<std::size_t> next{0};
    std::vector<std::future<void>> tasks;
    tasks.reserve(report.workers);

    for (std::size_t worker = 0; worker < report.workers; ++worker) {
        tasks.push_back(std::async(std::launch::async, [&report, &next] {
            while (true) {
                const std::size_t index = next.fetch_add(1, std::memory_order_relaxed);
                if (index >= report.entries.size()) return;
                Entry& entry = report.entries[index];
                // Each worker owns one result slot. Symlinks are classified, not followed.
                entry.type = std::filesystem::symlink_status(entry.path, entry.error).type();
            }
        }));
    }
    // Futures propagate unexpected worker exceptions and wait on destruction.
    for (auto& task : tasks) task.get();
    return report;
}
} // namespace inspection
