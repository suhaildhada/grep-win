#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <getopt.h>

#include "re.h"

#define MAX_BUFFER 1024

#ifdef _WIN32
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif


static void print_usage(void) {
    printf("Usage: grep_win [OPTIONS] PATTERN [FILE|DIR]...\n");
    printf("Options:\n");
    printf("  -i, --ignore-case    Ignore case distinctions\n");
}

static int is_regular_file(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0) && S_ISREG(st.st_mode);
}

static int is_directory(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0) && S_ISDIR(st.st_mode);
}

static char *build_path(const char *dir, const char *name) {
    size_t len = strlen(dir) + strlen(PATH_SEP) + strlen(name) + 1;
    char *path = malloc(len);
    if (!path) return NULL;

    snprintf(path, len, "%s%s%s", dir, PATH_SEP, name);
    return path;
}

static void search_file(const char *file_path, re_t pattern) {
    FILE *fp = fopen(file_path, "r");
    if (!fp) {
        fprintf(stderr, "Could not open file: %s\n", file_path);
        return;
    }

    char buffer[MAX_BUFFER];
    int line_no = 1;

    while (fgets(buffer, sizeof(buffer), fp)) {
        buffer[strcspn(buffer, "\n")] = '\0';

        int match_len;
        int match_idx = re_matchp(pattern, buffer, &match_len);
        if (match_idx != -1) {
            printf("%s:%d:%d\n", file_path, line_no, match_idx + 1);
        }
        line_no++;
    }

    fclose(fp);
}


static void search_directory(const char *path, re_t pattern) {
    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "Could not open directory: %s\n", path);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        char *full_path = build_path(path, entry->d_name);
        if (!full_path)
            continue;

        if (is_directory(full_path)) {
            search_directory(full_path, pattern);
        } else if (is_regular_file(full_path)) {
            search_file(full_path, pattern);
        }

        free(full_path);
    }

    closedir(dir);
}

static struct option long_options[] = {
    {"ignore-case", no_argument, nullptr, 'i'},
};

int main(int argc, char *argv[]) {
    int ignore_case = 0;

    static struct option long_options[] = {
        {"ignore-case", no_argument, nullptr, 'i'},
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "i", long_options, NULL)) != -1) {
        switch (opt) {
            case 'i':
                ignore_case = 1;
                break;
            default:
                print_usage();
                return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "No pattern provided\n");
        print_usage();
        return EXIT_FAILURE;
    }

    const char *pattern_str = argv[optind++];
    re_t pattern = re_compile(pattern_str);

    if (optind == argc) {
        search_directory(".", pattern);
    } else {
        for (int i = optind; i < argc; i++) {
            if (is_directory(argv[i])) {
                search_directory(argv[i], pattern);
            } else if (is_regular_file(argv[i])) {
                search_file(argv[i], pattern);
            }
        }
    }

    return EXIT_SUCCESS;
}
