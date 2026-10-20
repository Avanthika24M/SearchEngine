CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wconversion -O2 -Iinclude
LDLIBS = -lm
CORE = src/index.c src/storage.c
EXE =
ifeq ($(OS),Windows_NT)
EXE = .exe
endif
APP = build/search_app$(EXE)
TEST = build/test_search$(EXE)
.PHONY: all test clean debug sanitize coverage analyze
all: $(APP)
build:
	mkdir -p build
$(APP): src/main.c $(CORE) include/search.h | build
	$(CC) $(CFLAGS) -o $@ src/main.c $(CORE) $(LDLIBS)
$(TEST): test/test_search.c $(CORE) include/search.h | build
	$(CC) $(CFLAGS) -o $@ test/test_search.c $(CORE) $(LDLIBS)
test: $(TEST)
	./$(TEST)
debug: CFLAGS += -g -O0
debug: clean all
sanitize: CFLAGS += -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer
sanitize: LDLIBS += -fsanitize=address,undefined
sanitize: clean $(TEST)
	./$(TEST)
coverage: CFLAGS += -O0 -g --coverage
coverage: LDLIBS += --coverage
coverage: clean test
	gcov -b -c build/test_search-index.gcda build/test_search-storage.gcda || true
analyze:
	cppcheck --enable=warning,performance,portability --std=c11 --error-exitcode=1 -Iinclude src test
clean:
	rm -f build/search_app build/search_app.exe build/test_search build/test_search.exe build/*.gcda build/*.gcno
