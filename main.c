#include <stdio.h>
#include<assert.h>
#include <stdlib.h>
#include<dirent.h>
#include<sys/types.h>
#include <sys/stat.h>
#include <string.h>

#define shift(xs, xs_sz) (assert((xs_sz) > 0), (xs_sz)--, *(xs)++)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MAX_BUFFER 1024


int case_sensitive = 0;
const char* pattern;

void print_usage() {
    printf("Usage: ");
    printf("grep_win [OPTIONS]... PATTERNS [FILE]...");

}

int is_regular_file(const char* path) {
    struct stat st;
    stat(path, &st);
    return S_ISREG(st.st_mode);
}

int is_directory(const char* path) {
    struct stat st;
    stat(path, &st);
    return S_ISDIR(st.st_mode);
}

char* get_full_path(const char* path, const char* filename) {
    char* full_path = malloc(strlen(path) + strlen(filename) + 1);
    strcpy(full_path, path);
    strcat(full_path, "\\");
    strcat(full_path, filename);
}

void open_folder_rec(const char* path) {
    DIR *dir = opendir(path);

    if (dir == NULL) {
        printf("Could not open directory %s\n", path);
        return;
    }
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        const char* name = entry->d_name;

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        if (is_directory(name)) {
            open_folder_rec(name);
        } else {
            if (strcmp(name, "test_file.txt") != 0) {
                continue;
            }
            char* full_path = get_full_path(path, name);

            FILE *f = fopen(full_path, "r");

            if (f == NULL) {
                printf("Cound not open file: %s\n", full_path);
                free(full_path);
                return;
            }
            free(full_path);

            char buffer[MAX_BUFFER];
            while (fgets(buffer, MAX_BUFFER, f)) {
                buffer[strcspn(buffer, "\n")] = 0;
                printf("%s\n", buffer);
            }

            fclose(f);
        }
    }
}

int main(int argc, char *argv[]) {


    char* file_name = shift(argv, argc);

    if (argc == 0) {
        print_usage();
        return 1;
    }

    if (argc > 1) {
        //Parse options
    }

    pattern = shift(argv, argc);


    if (argc == 0) {
        char* current_path = getcwd(nullptr, 0);
        printf("current_path: %s\n", current_path);
        open_folder_rec(current_path);
        free(current_path);


    } else {

    }

    return 0;
}