#include "unity/unity.h"
#include "assembler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

void setUp(void) {
    // Called before each test
}

void tearDown(void) {
    // Called after each test
}

void test_all_samples(void) {
    const char *folder = "sample/";
    DIR *dir = opendir(folder);
    if (!dir) {
        perror("Failed to open sample folder");
        TEST_FAIL_MESSAGE("Could not open 'sample/' directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;
        size_t len = strlen(name);

        if (len > 5 && strcmp(name + len - 5, ".masm") == 0) {
            char path[256];
            snprintf(path, sizeof(path), "%s%s", folder, name);
            printf("Testing file: %s\n", path);
            assembler(path);  // Run your assembler on it
        }
    }

    closedir(dir);
    TEST_PASS();  // Mark the test as passed if no crash/errors
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_all_samples);

    return UNITY_END();
}
