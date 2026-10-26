
#include <gtest/gtest.h>

#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>

extern "C" {
#include "search.h"
}

// =====================================================
// COMMON TEST SETUP
// =====================================================

class SearchTest : public ::testing::Test {
protected:
    SearchIndex index{};
    std::string folder;
    bool initialized = false;

    void SetUp() override {
        char pattern[] = "/tmp/search_gtest_XXXXXX";
        char* result = mkdtemp(pattern);

        ASSERT_NE(result, nullptr);
        folder = result;

        ASSERT_EQ(hs_init(&index), 0);
        initialized = true;
    }

    void TearDown() override {
        if (initialized) {
            hs_destroy(&index);
        }

        if (!folder.empty()) {
            std::string command = "rm -rf -- " + folder;
            std::system(command.c_str());
        }
    }

    // Create a file for testing.
    std::string createFile(
        const std::string& name,
        const std::string& content
    ) {
        std::string path = folder + "/" + name;

        std::ofstream file(path, std::ios::binary);
        file << content;
        file.close();

        return path;
    }

    // Search and return results.
    std::vector<SearchResult> search(const char* keyword) {
        SearchResult* results = nullptr;

        size_t count = hs_search(&index, keyword, &results);

        std::vector<SearchResult> output;

        if (results != nullptr) {
            output.assign(results, results + count);
        }

        std::free(results);
        return output;
    }
};

// =====================================================
// FUNCTIONAL VALIDATION TESTS
// =====================================================

// TEST 1: Index initialization
TEST_F(SearchTest, InitializeIndex) {
    EXPECT_EQ(hs_active_documents(&index), 0u);
}

// TEST 2: Supported file types
TEST_F(SearchTest, SupportedFile) {
    EXPECT_NE(hs_supported_file("sample.txt"), 0);
    EXPECT_NE(hs_supported_file("program.c"), 0);
}

// TEST 3: Unsupported file types
TEST_F(SearchTest, UnsupportedFile) {
    EXPECT_EQ(hs_supported_file("image.png"), 0);
}

// TEST 4: Add a valid document
TEST_F(SearchTest, AddDocument) {
    std::string path = createFile("sample.txt", "hello cricket");

    int id = hs_add(&index, path.c_str());

    ASSERT_GT(id, 0);
    EXPECT_EQ(hs_active_documents(&index), 1u);
}

// TEST 5: Reject a missing document
TEST_F(SearchTest, MissingDocument) {
    std::string path = folder + "/missing.txt";

    EXPECT_LT(hs_add(&index, path.c_str()), 0);
    EXPECT_EQ(hs_active_documents(&index), 0u);
}

// TEST 6: Search an existing keyword
TEST_F(SearchTest, SearchKeyword) {
    std::string path =
        createFile("sample.txt", "cricket match highlights");

    ASSERT_GT(hs_add(&index, path.c_str()), 0);

    auto results = search("cricket");

    EXPECT_EQ(results.size(), 1u);
}

// TEST 7: Search an unknown keyword
TEST_F(SearchTest, UnknownKeyword) {
    std::string path =
        createFile("sample.txt", "cricket match");

    ASSERT_GT(hs_add(&index, path.c_str()), 0);

    auto results = search("football");

    EXPECT_TRUE(results.empty());
}

// TEST 8: Case-insensitive search
TEST_F(SearchTest, CaseInsensitiveSearch) {
    std::string path =
        createFile("sample.txt", "Cricket Highlights");

    ASSERT_GT(hs_add(&index, path.c_str()), 0);

    auto results = search("CRICKET");

    EXPECT_EQ(results.size(), 1u);
}

// TEST 9: Empty search query
TEST_F(SearchTest, EmptyQuery) {
    auto results = search("");

    EXPECT_TRUE(results.empty());
}

// TEST 10: Search multiple documents
TEST_F(SearchTest, MultipleDocuments) {
    std::string first =
        createFile("first.txt", "cricket match");

    std::string second =
        createFile("second.txt", "cricket highlights");

    ASSERT_GT(hs_add(&index, first.c_str()), 0);
    ASSERT_GT(hs_add(&index, second.c_str()), 0);

    auto results = search("cricket");

    EXPECT_EQ(results.size(), 2u);
}

// TEST 11: Ranked search results
TEST_F(SearchTest, RankedResults) {
    std::string first =
        createFile("first.txt", "cricket");

    std::string second =
        createFile("second.txt", "cricket cricket cricket");

    ASSERT_GT(hs_add(&index, first.c_str()), 0);
    ASSERT_GT(hs_add(&index, second.c_str()), 0);

    auto results = search("cricket");

    ASSERT_EQ(results.size(), 2u);
    EXPECT_GE(results[0].score, results[1].score);
}

// TEST 12: Update a document
TEST_F(SearchTest, UpdateDocument) {
    std::string path =
        createFile("sample.txt", "cricket");

    int id = hs_add(&index, path.c_str());
    ASSERT_GT(id, 0);

    createFile("sample.txt", "football");

    ASSERT_EQ(hs_update(&index, (uint32_t)id), 0);

    EXPECT_EQ(search("football").size(), 1u);
    EXPECT_TRUE(search("cricket").empty());
}

// TEST 13: Delete a document
TEST_F(SearchTest, DeleteDocument) {
    std::string path =
        createFile("sample.txt", "cricket");

    int id = hs_add(&index, path.c_str());
    ASSERT_GT(id, 0);

    ASSERT_EQ(hs_delete(&index, (uint32_t)id), 0);

    EXPECT_EQ(hs_active_documents(&index), 0u);
    EXPECT_TRUE(search("cricket").empty());
}

// TEST 14: Save and reload the index
TEST_F(SearchTest, SaveAndReload) {
    std::string path =
        createFile("sample.txt", "cricket highlights");

    ASSERT_GT(hs_add(&index, path.c_str()), 0);

    std::string saveFolder = folder + "/saved";

    ASSERT_EQ(mkdir(saveFolder.c_str(), 0700), 0);
    ASSERT_EQ(hs_save(&index, saveFolder.c_str()), 0);

    // Release the current index before loading.
    hs_destroy(&index);
    initialized = false;

    ASSERT_EQ(hs_init(&index), 0);
    initialized = true;

    ASSERT_EQ(hs_load(&index, saveFolder.c_str()), 0);

    EXPECT_EQ(hs_active_documents(&index), 1u);
    EXPECT_EQ(search("cricket").size(), 1u);
}

// TEST 15: Initial statistics
TEST_F(SearchTest, EmptyIndexStatistics) {
    EXPECT_EQ(index.searches, 0u);
    EXPECT_EQ(index.unique_terms, 0u);
    EXPECT_EQ(hs_active_documents(&index), 0u);
}

// ADDITIONAL GTEST COVERAGE: result printing and storage edge cases.
TEST_F(SearchTest, PrintNoResults) {
    FILE* out = tmpfile();
    ASSERT_NE(out, nullptr);
    hs_print_results(&index, nullptr, 0, out);
    rewind(out);
    char line[256] = {};
    ASSERT_NE(fgets(line, sizeof(line), out), nullptr);
    EXPECT_NE(std::string(line).find("No matching documents."), std::string::npos);
    fclose(out);
}

TEST_F(SearchTest, PrintMatchingResults) {
    auto path = createFile("print.txt", "cricket cricket");
    ASSERT_GT(hs_add(&index, path.c_str()), 0);
    auto results = search("cricket");
    ASSERT_EQ(results.size(), 1u);
    FILE* out = tmpfile();
    ASSERT_NE(out, nullptr);
    hs_print_results(&index, results.data(), results.size(), out);
    rewind(out);
    char line[2048] = {};
    ASSERT_NE(fgets(line, sizeof(line), out), nullptr);
    EXPECT_NE(std::string(line).find("print.txt"), std::string::npos);
    fclose(out);
}

TEST_F(SearchTest, PrintResultsWithNullStream) {
    hs_print_results(&index, nullptr, 0, nullptr);
    SUCCEED();
}

TEST_F(SearchTest, InvalidPersistenceArguments) {
    EXPECT_EQ(hs_save(nullptr, folder.c_str()), -1);
    EXPECT_EQ(hs_save(&index, nullptr), -1);
    EXPECT_EQ(hs_load(nullptr, folder.c_str()), -1);
    EXPECT_EQ(hs_load(&index, nullptr), -1);
}

TEST_F(SearchTest, MissingDatabaseIsEmpty) {
    EXPECT_EQ(hs_load(&index, folder.c_str()), 0);
    EXPECT_EQ(hs_active_documents(&index), 0u);
}

TEST_F(SearchTest, RejectInvalidDatabaseHeader) {
    auto path = createFile("documents.dat", "BAD_HEADER\n1\n");
    (void)path;
    EXPECT_EQ(hs_load(&index, folder.c_str()), -1);
}

// =====================================================
// INTENTIONAL FAILURE DEMONSTRATIONS
// Disabled during normal execution.
// =====================================================

// TEST 16: Intentionally incorrect file expectation
TEST(FailureDemo, DISABLED_WrongFileExtension) {
    // PNG is unsupported, but we deliberately expect 1.
    EXPECT_EQ(hs_supported_file("image.png"), 1);
}

// TEST 17: Intentionally incorrect document count
TEST(FailureDemo, DISABLED_WrongDocumentCount) {
    SearchIndex index{};

    ASSERT_EQ(hs_init(&index), 0);

    // A new index contains 0 documents, not 5.
    EXPECT_EQ(hs_active_documents(&index), 5u);

    hs_destroy(&index);
}

// =====================================================
// GOOGLE TEST MAIN FUNCTION
// =====================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
