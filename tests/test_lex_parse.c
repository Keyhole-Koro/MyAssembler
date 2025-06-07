#include "unity/unity.h"
#include "assembler.h"

#include <stdio.h>
#include <stdlib.h>


void setUp(void) {
    // This function is called before each test
}

void tearDown(void) {
    // This function is called after each test
}

void test_parser_sample1(void) {
    const char *file_path = "sample/label.masm";
    assembler(file_path);

}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_parser_sample1);

    return UNITY_END();
}