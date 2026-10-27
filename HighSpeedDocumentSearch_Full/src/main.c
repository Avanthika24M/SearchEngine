
#include "search.h"

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define MKDIR(p) mkdir((p), 0700U)
#endif

/* Print a message and check for output errors. */
static void safe_puts(const char *message)
{
    if (puts(message) == EOF) {
        (void)fputs("Output error\n", stderr);
    }
}

/* Print formatted output and check for errors. */
static void safe_printf(const char *format, ...)
{
    va_list args;
    int result;

    va_start(args, format);
    result = vprintf(format, args);
    va_end(args);

    if (result < 0) {
        (void)fputs("Output error\n", stderr);
    }
}

/* Write to a specified stream and check for errors. */
static void safe_fputs(const char *message, FILE *stream)
{
    if (fputs(message, stream) == EOF) {
        if (stream != stderr) {
            (void)fprintf(stderr, "Error: failed to write output\n");
        }
    }
}


/* Print the current date, time and operation status. */
static void print_timestamp(const char *message)
{
    time_t now = time(NULL);
    const struct tm *current_time = localtime(&now);
    char timestamp[32];

    if ((current_time == NULL) ||
        (strftime(timestamp, sizeof(timestamp),
                  "%Y-%m-%d %H:%M:%S", current_time) == 0U)) {

        safe_printf("[Time unavailable] %s\n", message);
        return;
    }

    safe_printf("[%s] %s\n", timestamp, message);
}

/* Read a complete input line. */
static int read_line(char *s, size_t cap)
{
    if ((s == NULL) || (cap == 0U) || (cap > (size_t)INT_MAX)) {
        return -1;
    }

    if (fgets(s, (int)cap, stdin) == NULL) {
        return -1;
    }

    size_t n = strcspn(s, "\n");

    if (s[n] == '\n') {
        s[n] = '\0';
    } else {
        int c;

        do {
            c = getchar();
        } while ((c != '\n') && (c != EOF));

        return -1;
    }

    return 0;
}

/* Validate a document ID. */
static int parse_id(const char *s, uint32_t *out)
{
    char *end = NULL;
    unsigned long n;

    if ((s == NULL) || (*s == '\0') ||
        (*s == '-') || (out == NULL)) {
        return -1;
    }

    errno = 0;
    n = strtoul(s, &end, 10);

    if ((errno != 0) || (*end != '\0') ||
        (n == 0UL) || (n > UINT32_MAX)) {
        return -1;
    }

    *out = (uint32_t)n;
    return 0;
}

/* Display active indexed documents. */
static void list(const SearchIndex *idx)
{
    if (idx == NULL) {
        return;
    }

    for (size_t i = 0U; i < idx->count; i++) {
        if (idx->docs[i].active != 0) {
            safe_printf(
                "[%u] %s (%llu bytes, %u words)\n",
                idx->docs[i].id,
                idx->docs[i].path,
                (unsigned long long)idx->docs[i].size,
                idx->docs[i].words
            );
        }
    }
}

/* Store a search query in the history file. */
static void history(const char *dir,
                    const char *query,
                    size_t results)
{
    char path[HS_PATH_MAX + 64];
    int n = snprintf(path, sizeof(path),
                     "%s/history.dat", dir);

    if ((n <= 0) || ((size_t)n >= sizeof(path))) {
        return;
    }

    FILE *f = fopen(path, "ab");

    if (f != NULL) {
        int written = fprintf(
            f,
            "%lld\t%zu\t%s\n",
            (long long)time(NULL),
            results,
            query
        );

        if (written < 0) {
            safe_fputs("Warning: history write failed\n", stderr);
        }

        if (fclose(f) != 0) {
            safe_fputs("Warning: history close failed\n", stderr);
        }
    }
}

/* Display previous search queries. */
static void show_history(const char *dir)
{
    char path[HS_PATH_MAX + 64];
    int n = snprintf(path, sizeof(path),
                     "%s/history.dat", dir);

    if ((n <= 0) || ((size_t)n >= sizeof(path))) {
        return;
    }

    FILE *f = fopen(path, "rb");

    if (f == NULL) {
        safe_puts("No history yet.");
        return;
    }

    char line[HS_PATH_MAX + 128];

    while (fgets(line, sizeof(line), f) != NULL) {
        safe_fputs(line, stdout);
    }

    if (ferror(f) != 0) {
        safe_fputs("Warning: history read failed\n", stderr);
    }

    if (fclose(f) != 0) {
        safe_fputs("Warning: history close failed\n", stderr);
    }
}

int main(int argc, char **argv)
{
    print_timestamp("Application started");

    const char *dir = (argc > 1) ? argv[1] : "build";

    if ((MKDIR(dir) != 0) && (errno != EEXIST)) {
        perror("storage directory");
        print_timestamp("ERROR: Storage directory creation failed");
        return 1;
    }

    SearchIndex idx;

    if (hs_init(&idx) != 0) {
        safe_fputs("Initialization failed\n", stderr);
        print_timestamp("ERROR: Index initialization failed");
        return 1;
    }

    if (hs_load(&idx, dir) != 0) {
        safe_fputs("Could not load database\n", stderr);
        print_timestamp("ERROR: Database loading failed");
        hs_destroy(&idx);
        return 1;
    }

    print_timestamp("Database loaded successfully");

    char line[HS_PATH_MAX + 32];

    for (;;) {
        safe_puts("\n=== HIGH-SPEED DOCUMENT SEARCH ===");
        safe_puts("1 Add file     2 Search       3 Update ID    4 Delete ID");
        safe_puts("5 List         6 Statistics   7 Save         8 Query history");
        safe_puts("0 Exit");
        safe_puts("Choice:");

        if (read_line(line, sizeof(line)) != 0) {
            break;
        }

        char *end = NULL;
        long choice = strtol(line, &end, 10);

        if ((*end != '\0') || (end == line)) {
            safe_puts("Invalid choice");
            print_timestamp("WARNING: Invalid menu choice");
            continue;
        }

        if (choice == 0) {
            print_timestamp("Application exit requested");
            break;
        }

        if (choice == 1) {
            safe_puts("File path:");

            if (read_line(line, sizeof(line)) != 0) {
                continue;
            }

            int id = hs_add(&idx, line);

            if (id > 0) {
                safe_printf("Indexed ID: %u\n", (unsigned int)id);
                print_timestamp("SUCCESS: Document added");
            } else if (id == -2) {
                safe_puts(
                    "Already indexed: this file path exists in the index."
                );
                print_timestamp("WARNING: Document already indexed");
            } else {
                safe_puts(
                    "Add failed: check that the file exists, is readable, "
                    "is <=8 MiB, and has a supported extension."
                );
                print_timestamp("ERROR: Document addition failed");
            }
        }
        else if (choice == 2) {
            safe_puts("Search keywords:");

            if (read_line(line, sizeof(line)) != 0) {
                continue;
            }

            SearchResult *results = NULL;
            size_t n = hs_search(&idx, line, &results);

            safe_printf("%zu results\n", n);
            hs_print_results(&idx, results, n, stdout);

            free(results);
            history(dir, line, n);

            print_timestamp("Search completed");
        }
        else if ((choice == 3) || (choice == 4)) {
            safe_puts("Document ID:");

            if (read_line(line, sizeof(line)) != 0) {
                continue;
            }

            uint32_t id;

            if (parse_id(line, &id) != 0) {
                safe_puts("Invalid document ID");
                print_timestamp("ERROR: Invalid document ID");
                continue;
            }

            int rc = (choice == 3)
                ? hs_update(&idx, id)
                : hs_delete(&idx, id);

            if (rc == 0) {
                safe_puts("Operation completed");

                if (choice == 3) {
                    print_timestamp("SUCCESS: Document updated");
                } else {
                    print_timestamp("SUCCESS: Document deleted");
                }
            } else {
                safe_puts(
                    "Operation failed: check ID and file accessibility"
                );

                if (choice == 3) {
                    print_timestamp("ERROR: Document update failed");
                } else {
                    print_timestamp("ERROR: Document deletion failed");
                }
            }
        }
        else if (choice == 5) {
            list(&idx);
            print_timestamp("Document list displayed");
        }
        else if (choice == 6) {
            safe_printf(
                "Active documents: %zu, keywords: %zu, "
                "searches: %llu, avg CPU ms: %.3f, "
                "posting visits: %llu\n",
                hs_active_documents(&idx),
                idx.unique_terms,
                (unsigned long long)idx.searches,
                idx.searches
                    ? idx.total_ms / (double)idx.searches
                    : 0.0,
                (unsigned long long)idx.candidate_visits
            );

            print_timestamp("Statistics displayed");
        }
        else if (choice == 7) {
            if (hs_save(&idx, dir) == 0) {
                safe_puts("Saved");
                print_timestamp("SUCCESS: Index saved");
            } else {
                safe_puts("Save failed");
                print_timestamp("ERROR: Index saving failed");
            }
        }
        else if (choice == 8) {
            show_history(dir);
            print_timestamp("Query history displayed");
        }
        else {
            safe_puts("Invalid choice");
            print_timestamp("WARNING: Invalid menu choice");
        }
    }

    if (hs_save(&idx, dir) != 0) {
        safe_fputs("Warning: save failed\n", stderr);
        print_timestamp("WARNING: Final index save failed");
    } else {
        print_timestamp("Final index saved");
    }

    hs_destroy(&idx);
    print_timestamp("Application closed");

    return 0;
}
