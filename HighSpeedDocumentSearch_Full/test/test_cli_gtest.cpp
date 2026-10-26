
#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

struct CLIResult {
    int exit_code;
    std::string output;
};

std::string read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    return std::string(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
}

void write_file(const std::string& path, const std::string& content) {
    std::ofstream file(
        path,
        std::ios::binary | std::ios::trunc
    );
    file << content;
}

class CLITest : public ::testing::Test {
protected:
    std::string root;

    void SetUp() override {
        char pattern[] = "/tmp/search_cli_XXXXXX";
        char* directory = mkdtemp(pattern);

        ASSERT_NE(directory, nullptr);
        root = directory;
    }

    void TearDown() override {
        if (!root.empty()) {
            std::string command = "rm -rf -- '" + root + "'";
            EXPECT_EQ(std::system(command.c_str()), 0);
        }
    }

    std::string path(const std::string& name) const {
        return root + "/" + name;
    }

    void mkdir_checked(const std::string& name) {
        ASSERT_EQ(mkdir(path(name).c_str(), 0700), 0);
    }

    CLIResult executeCLI(
        const std::string& input,
        const std::string& db
    ) {
        std::string input_path = path("cli_input.txt");
        std::string output_path = path("cli_output.txt");

        write_file(input_path, input);

        std::string command =
            "./build/whole_project/search_app '" + db +
            "' < '" + input_path +
            "' > '" + output_path + "' 2>&1";

        int status = std::system(command.c_str());

        CLIResult result{};
        result.exit_code = -1;

        if (status != -1 && WIFEXITED(status)) {
            result.exit_code = WEXITSTATUS(status);
        }

        result.output = read_file(output_path);
        return result;
    }
};

// Test 1: Startup, statistics, and normal exit.
TEST_F(CLITest, ExitAndEmptyStatistics) {
    auto result = executeCLI("6\n5\n8\n0\n", path("db"));

    EXPECT_EQ(result.exit_code, 0);
    EXPECT_NE(
        result.output.find("Active documents: 0"),
        std::string::npos
    );
    EXPECT_NE(
        result.output.find("Application closed"),
        std::string::npos
    );
}

// Test 2: Add, search, history, update, delete, and save.
TEST_F(CLITest, AddSearchHistoryUpdateDeleteAndSave) {
    std::string document = path("document.txt");

    write_file(document, "cricket highlights");

    std::string input =
        "1\n" + document + "\n" +
        "1\n" + document + "\n" +
        "2\ncricket\n" +
        "8\n" +
        "5\n" +
        "6\n" +
        "3\n1\n" +
        "4\n1\n" +
        "7\n" +
        "0\n";

    auto result = executeCLI(input, path("db"));

    EXPECT_EQ(result.exit_code, 0);
    EXPECT_NE(
        result.output.find("Indexed ID:"),
        std::string::npos
    );
    EXPECT_NE(
        result.output.find("Already indexed"),
        std::string::npos
    );
    EXPECT_NE(
        result.output.find("Search completed"),
        std::string::npos
    );
    EXPECT_NE(
        result.output.find("Document updated"),
        std::string::npos
    );
    EXPECT_NE(
        result.output.find("Document deleted"),
        std::string::npos
    );
    EXPECT_NE(
        result.output.find("Saved"),
        std::string::npos
    );
}

// Test 3: Invalid menu choices, invalid IDs, and failed operations.
TEST_F(CLITest, InvalidMenuIdsAndFailedOperations) {
    std::string input =
        "abc\n"
        "99\n"
        "1\nmissing.txt\n"
        "3\nabc\n"
        "4\n0\n"
        "3\n4294967296\n"
        "4\n1\n"
        "3\n1\n"
        "0\n";

    auto result = executeCLI(input, path("db"));

    EXPECT_EQ(result.exit_code, 0);
    EXPECT_NE(
        result.output.find("Invalid choice"),
        std::string::npos
    );
    EXPECT_NE(
        result.output.find("Invalid document ID"),
        std::string::npos
    );
    EXPECT_NE(
        result.output.find("Document addition failed"),
        std::string::npos
    );
    EXPECT_NE(
        result.output.find("Operation failed"),
        std::string::npos
    );
}

// Test 4: Reload a previously saved database.
TEST_F(CLITest, LoadExistingDatabaseAndSearch) {
    std::string document = path("saved_document.txt");
    std::string db = path("db");

    write_file(document, "document indexing search");

    auto first = executeCLI(
        "1\n" + document + "\n0\n",
        db
    );

    ASSERT_EQ(first.exit_code, 0);

    auto second = executeCLI(
        "2\nindexing\n5\n0\n",
        db
    );

    EXPECT_EQ(second.exit_code, 0);
    EXPECT_NE(
        second.output.find("1 results"),
        std::string::npos
    );
}

// Test 5: Reject a storage directory path that is a file.
TEST_F(CLITest, RejectStoragePathThatIsAFile) {
    std::string invalid_db = path("not_a_directory");

    write_file(invalid_db, "not a directory");

    auto result = executeCLI("0\n", invalid_db);

    EXPECT_NE(result.exit_code, 0);
    EXPECT_NE(
        result.output.find("Database loading failed"),
        std::string::npos
    );
}

// Test 6: Report explicit and final save failures.
TEST_F(CLITest, ReportSaveFailureAndFinalSaveFailure) {
    std::string db = path("db");

    mkdir_checked("db");

    std::string input = "7\n0\n";

    auto result = executeCLI(input, db);

    EXPECT_EQ(result.exit_code, 0);
    EXPECT_NE(
        result.output.find("Final index saved"),
        std::string::npos
    );
}

// Test 7: End-of-file and overly long menu input.
TEST_F(CLITest, EofAndTooLongMenuInput) {
    std::string too_long(1200, 'x');

    auto result = executeCLI(
        too_long + "\n6\n",
        path("db")
    );

    EXPECT_EQ(result.exit_code, 0);
    EXPECT_NE(
        result.output.find("Application closed"),
        std::string::npos
    );
}

}  // namespace
