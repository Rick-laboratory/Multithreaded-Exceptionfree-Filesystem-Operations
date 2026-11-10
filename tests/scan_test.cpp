#include "scan.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>

namespace {
namespace fs = std::filesystem;
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Fixture {
    fs::path base = fs::canonical(fs::current_path());
    fs::path root = base / ("scan-test-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    bool owned = false;

    Fixture() {
        owned = fs::create_directory(root);
        require(owned, "Cannot create isolated fixture directory.");
    }
    ~Fixture() {
        std::error_code error;
        const auto resolved = fs::weakly_canonical(root, error);
        // Delete only our newly created child of this test's working directory.
        if (owned && !error && resolved.parent_path() == base &&
            resolved.filename() == root.filename()) {
            fs::remove_all(root, error);
        }
    }
};
} // namespace

int main() {
    try {
        Fixture fixture;
        fs::create_directory(fixture.root / "nested");
        fs::create_directory(fixture.root / "empty");
        std::set<fs::path> expected;
        for (int index = 0; index < 257; ++index) {
            const fs::path file = fixture.root / "nested" / (std::to_string(index) + ".txt");
            std::ofstream(file) << "fixture";
            expected.insert(file);
        }
        const fs::path unicode = fixture.root / fs::path(u"unicode-\u00E4-\u6587.txt");
        std::ofstream(unicode) << "unicode";
        expected.insert(unicode);

        for (const unsigned workers : {0U, 1U, 2U, 4U, 1000U}) {
            const auto report = inspection::scan(fixture.root, workers);
            require(!report.traversal_error, "Traversal unexpectedly failed.");
            require(report.workers >= 1 && report.workers <= 32, "Worker bounds violated.");
            std::set<fs::path> actual;
            for (const auto& entry : report.entries) {
                require(!entry.error, "Metadata inspection failed.");
                if (entry.type == fs::file_type::regular) actual.insert(entry.path);
            }
            require(actual == expected, "A worker partition lost a file or damaged its path.");
        }

        const auto empty = inspection::scan(fixture.root / "empty", 0);
        require(empty.entries.empty() && empty.workers == 0 && !empty.traversal_error,
                "Empty directories were not handled correctly.");
        const auto missing = inspection::scan(fixture.root / "missing", 4);
        require(static_cast<bool>(missing.traversal_error), "Missing directory error was lost.");
        const auto file_root = inspection::scan(unicode, 4);
        require(static_cast<bool>(file_root.traversal_error), "File accepted as a directory root.");
        std::cout << "Complete coverage, zero/large worker counts, Unicode and error checks passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
