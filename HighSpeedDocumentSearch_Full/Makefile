
# ============================================================
# HIGH-SPEED DOCUMENT SEARCH FRAMEWORK
# Build and Testing Makefile
# ============================================================

# 1. Compilers and flags
CC       = gcc
CXX      = g++
CSTD     = -std=c11
CXXSTD   = -std=c++17
WARN     = -Wall -Wextra -Wpedantic -Wconversion
INC      = -Iinclude
OPT      = -O2
LDLIBS   = -lm
GTESTLIB = -lgtest -pthread

# 2. Project files
BUILD    = build
CORE     = src/index.c src/storage.c
HEADER   = include/search.h

APP      = $(BUILD)/search_app
CTEST    = $(BUILD)/test_search
GTEST    = $(BUILD)/test_gtest

# 3. Main targets
.PHONY: all run test gtest gtest-fail sanitize valgrind \
        helgrind cppcheck coverage comprehensive coverage-comprehensive clean help

all: $(APP)

$(BUILD):
	mkdir -p $(BUILD)

# 4. Main application
$(APP): src/main.c $(CORE) $(HEADER) | $(BUILD)
	$(CC) $(CSTD) $(WARN) $(OPT) $(INC) \
	    -o $@ src/main.c $(CORE) $(LDLIBS)

run: $(APP)
	./$(APP)

# 5. Existing C tests
$(CTEST): test/test_search.c $(CORE) $(HEADER) | $(BUILD)
	$(CC) $(CSTD) $(WARN) $(OPT) $(INC) \
	    -o $@ test/test_search.c $(CORE) $(LDLIBS)

test: $(CTEST)
	./$(CTEST)

# 6. Google Test (C source compiled separately)
$(BUILD)/index_gtest.o: src/index.c $(HEADER) | $(BUILD)
	$(CC) $(CSTD) -O0 -g $(INC) -c $< -o $@

$(BUILD)/storage_gtest.o: src/storage.c $(HEADER) | $(BUILD)
	$(CC) $(CSTD) -O0 -g $(INC) -c $< -o $@

$(GTEST): test/test_search_gtest.cpp \
          $(BUILD)/index_gtest.o \
          $(BUILD)/storage_gtest.o
	$(CXX) $(CXXSTD) -O0 -g $(INC) \
	    -o $@ test/test_search_gtest.cpp \
	    $(BUILD)/index_gtest.o \
	    $(BUILD)/storage_gtest.o \
	    $(GTESTLIB) $(LDLIBS)

gtest: $(GTEST)
	./$(GTEST) --gtest_output=xml:gtest_results.xml

# Run intentionally failing demonstration tests
gtest-fail: $(GTEST)
	-./$(GTEST) --gtest_also_run_disabled_tests \
	    --gtest_filter='FailureDemo.*'

# 7. AddressSanitizer and UndefinedBehaviorSanitizer
sanitize: | $(BUILD)
	$(CC) $(CSTD) $(WARN) -O1 -g $(INC) \
	    -fsanitize=address,undefined \
	    -fno-omit-frame-pointer \
	    -o $(BUILD)/test_sanitize \
	    test/test_search.c $(CORE) $(LDLIBS)
	./$(BUILD)/test_sanitize

# 8. Valgrind memory validation
valgrind: $(CTEST)
	valgrind --leak-check=full \
	    --show-leak-kinds=all \
	    --track-origins=yes \
	    --error-exitcode=1 \
	    ./$(CTEST)

# 9. Helgrind thread validation
helgrind: $(CTEST)
	valgrind --tool=helgrind \
	    --error-exitcode=1 \
	    ./$(CTEST)

# 10. Static code analysis
cppcheck:
	cppcheck --enable=warning,performance,portability \
	    --std=c11 --error-exitcode=1 \
	    -Iinclude src test

# 11. GCOV code coverage
coverage:
	@mkdir -p build
	$(CC) -std=c11 -O0 -g --coverage -Iinclude \
		-o build/test_coverage \
		test/test_search.c src/index.c src/storage.c -lm
	./build/test_coverage
	gcov -b -c -o build/test_coverage-index.gcno src/index.c
	gcov -b -c -o build/test_coverage-storage.gcno src/storage.c

# 12. Cleanup
clean:
	rm -rf $(BUILD)/search_app \
	       $(BUILD)/test_search \
	       $(BUILD)/test_gtest \
	       $(BUILD)/index_gtest.o \
	       $(BUILD)/storage_gtest.o \
	       $(BUILD)/test_sanitize \
	       $(BUILD)/test_coverage
	rm -f *.gcov $(BUILD)/*.gcda $(BUILD)/*.gcno
	rm -f $(BUILD)/test_comprehensive $(BUILD)/test_comprehensive_coverage

# 13. Help menu
help:
	@echo "High-Speed Document Search Framework"
	@echo "------------------------------------"
	@echo "make              - Build application"
	@echo "make run          - Run application"
	@echo "make test         - Run C tests"
	@echo "make gtest        - Run Google Tests"
	@echo "make gtest-fail   - Run intentional failure demos"
	@echo "make sanitize     - Run ASan and UBSan"
	@echo "make valgrind     - Run memory leak checks"
	@echo "make helgrind     - Run thread checks"
	@echo "make cppcheck     - Run static analysis"
	@echo "make coverage     - Run original C-test GCOV coverage"
	@echo "make comprehensive - Run extended API and error-path tests"
	@echo "make coverage-comprehensive - Run extended GCOV coverage"
	@echo "make clean        - Remove build artifacts"

# 14. Extended API, negative-path and corrupted-storage tests
$(BUILD)/test_comprehensive: test/test_comprehensive.c $(CORE) $(HEADER) | $(BUILD)
	$(CC) $(CSTD) $(WARN) $(OPT) $(INC) \
	    -o $@ test/test_comprehensive.c $(CORE) $(LDLIBS)

comprehensive: $(BUILD)/test_comprehensive
	./$(BUILD)/test_comprehensive

# Clean old profiling counters to avoid stale-checksum warnings.
coverage-comprehensive: | $(BUILD)
	rm -f $(BUILD)/test_comprehensive_coverage-*.gcda $(BUILD)/test_comprehensive_coverage-*.gcno
	$(CC) $(CSTD) -O0 -g --coverage $(INC) \
	    -o $(BUILD)/test_comprehensive_coverage \
	    test/test_comprehensive.c $(CORE) $(LDLIBS)
	./$(BUILD)/test_comprehensive_coverage
	gcov -b -c -o $(BUILD)/test_comprehensive_coverage-index.gcno src/index.c
	gcov -b -c -o $(BUILD)/test_comprehensive_coverage-storage.gcno src/storage.c
