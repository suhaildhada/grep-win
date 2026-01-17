#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <windows.h>
#include <wchar.h>
#include <stdlib.h>

#include "re.h"

#define MAX_BUFFER 1024
#define MAX_LINE 4096

#define PATH_SEP "\\"


static const char *ignored_dirs[] = {
    ".git", "node_modules", "target", "build", NULL
};


static wchar_t* utf8_to_wide(const char* s) {
    int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    if (len == 0) return NULL;

    wchar_t* ws = malloc(len * sizeof(wchar_t));
    if (!ws) return NULL;

    MultiByteToWideChar(CP_UTF8, 0, s, -1, ws, len);
    return ws;
}

static void wide_to_utf8(const wchar_t* ws, char* out, size_t out_sz) {
    WideCharToMultiByte(
        CP_UTF8, 0, ws, -1, out, (int) out_sz, NULL, NULL);
}

static void search_file_win32(const wchar_t* path, re_t pattern) {

    HANDLE hFile = CreateFileW(
        path,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        return;
    }

    LARGE_INTEGER size;

    if (!GetFileSizeEx(hFile, &size) || size.QuadPart == 0) {
        CloseHandle(hFile);
        return;
    }

    HANDLE hMap = CreateFileMappingW(
        hFile,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL
    );

    if (!hMap) {
        CloseHandle(hFile);
        return;
    }


    char *data = MapViewOfFile(
        hMap,
        FILE_MAP_READ,
        0, 0, 0
    );

    if (!data) {
        CloseHandle(hMap);
        CloseHandle(hFile);
        return;
    }

    size_t probe = size.QuadPart < MAX_LINE ? size.QuadPart : MAX_LINE;

    for (size_t i = 0; i < probe; i++) {
        if (data[i] == '\0') {
            goto cleanup;
        }
    }

    size_t pos = 0;
    size_t line_start = 0;
    int line_no = 1;

    char line[MAX_LINE];
    char utf8_path[MAX_PATH];

    wide_to_utf8(path, utf8_path, sizeof(utf8_path));

    while (pos <= (size_t) size.QuadPart) {
        if (pos == (size_t) size.QuadPart || data[pos] == '\n') {
            const size_t len = pos - line_start;

            if (len > 0 && len < MAX_LINE) {
                memcpy(line, data + line_start, len);
                line[len] = '\0';

                int match_len;
                const int match_idx = re_matchp(pattern, line, &match_len);
                if (match_idx != -1) {
                    printf("%s:%d:%d\n", utf8_path, line_no, match_idx + 1);
                    break;
                }
            }
            line_start = pos + 1;
            line_no++;
        }
        pos++;
    }


    cleanup:
        UnmapViewOfFile(data);
        CloseHandle(hMap);
        CloseHandle(hFile);
}


static int should_skip_dir(const wchar_t* name) {
    static const wchar_t *skip[] = {
        L".git", L".node_modules", L"target", L"build", nullptr
    };


    for (int i = 0; skip[i]; i++) {
        if (_wcsicmp(name, skip[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

static void search_dir(const wchar_t *path, re_t pattern) {
    wchar_t search[MAX_PATH];
    WIN32_FIND_DATAW fd;

    _snwprintf_s(search, MAX_PATH, _TRUNCATE, L"%s\\*", path);


    HANDLE hFind = FindFirstFileW(search, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        const wchar_t *name = fd.cFileName;

        if (wcscmp(name, L".") == 0 || wcscmp(name, L"..") == 0) {
            continue;
        }

        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && should_skip_dir(name)) {
            continue;
        }

        wchar_t full[MAX_PATH];
        _snwprintf_s(full, MAX_PATH, _TRUNCATE, L"%s\\%s", path, name);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            search_dir(full, pattern);
        } else {
            search_file_win32(full, pattern);
        }

    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Usage: grep_win PATTERN\n");
        return 1;
    }

    re_t pattern = re_compile(argv[1] );

    wchar_t cwd[MAX_PATH];
    if (!GetCurrentDirectoryW(MAX_PATH, cwd)) {
        fprintf(stderr, "Failed to get current directory\n");
        return 1;
    }

    search_dir(cwd, pattern);

    return 0;
}
