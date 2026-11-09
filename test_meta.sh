#!/bin/bash

GREEN="\e[32m"
RED="\e[31m"
RESET="\e[0m"

echo "Running Shiro compiler meta-writer tests..."

assert_meta_section_contains() {
    local code="$1"
    local section="$2"
    local expected_pattern="$3"

    echo "$code" > tmp_meta_test.shiro
    ./shiro tmp_meta_test.shiro -M > /dev/null 2>&1
    local status=$?

    if [ $status -ne 0 ]; then
        echo -e "${RED}[FAIL] Meta generation failed for: \"$code\"${RESET}"
        rm -f tmp_meta_test.shiro tmp_meta_test.shiro.meta
        exit 1
    fi

    # Extract specific section content
    local section_content=""
    if [ "$section" == "public" ]; then
        section_content=$(sed -n '/\/\/ public definition/,/\/\/ private definition/p' tmp_meta_test.shiro.meta)
    else
        section_content=$(sed -n '/\/\/ private definition/,$p' tmp_meta_test.shiro.meta)
    fi

    if echo "$section_content" | grep -q "$expected_pattern"; then
        echo -e "${GREEN}[OK]   [$section section] contains: \"$expected_pattern\"${RESET}"
    else
        echo -e "${RED}[FAIL] Pattern \"$expected_pattern\" NOT found in $section section!${RESET}"
        echo "--- Generated Meta File ---"
        cat tmp_meta_test.shiro.meta
        echo "---------------------------"
        rm -f tmp_meta_test.shiro tmp_meta_test.shiro.meta
        exit 1
    fi

    rm -f tmp_meta_test.shiro tmp_meta_test.shiro.meta
}

# --- Test Cases ---

# 1. Header Information (Whole file check)
assert_meta_contains() {
    local code="$1"
    local expected_pattern="$2"
    echo "$code" > tmp_meta_test.shiro
    ./shiro tmp_meta_test.shiro -M > /dev/null 2>&1
    if grep -q "$expected_pattern" tmp_meta_test.shiro.meta; then
        echo -e "${GREEN}[OK]   [file header] contains: \"$expected_pattern\"${RESET}"
    else
        echo -e "${RED}[FAIL] Pattern \"$expected_pattern\" NOT found in meta file!${RESET}"
        rm -f tmp_meta_test.shiro tmp_meta_test.shiro.meta
        exit 1
    fi
    rm -f tmp_meta_test.shiro tmp_meta_test.shiro.meta
}

assert_meta_contains "fn main() -> i8 { 0; }" "// shiro-interface"
assert_meta_contains "fn main() -> i8 { 0; }" "// source_hash:"

# 2. Private Functions
assert_meta_section_contains "fn add(x: i32, y: i32) -> i32 { x + y; }" "private" "fn add(x: i32, y: i32) -> i32;"

# 3. Private Structs
assert_meta_section_contains "struct Point { x: i8, y: i8 };" "private" "struct Point {"

# 4. Private Methods
assert_meta_section_contains "struct Point { x: i8, fn getx() -> i8 { return self.x; } }; fn main() -> i8 { 0; }" "private" "fn getx(self: &Point) -> i8;"

# 5. pub propagation on struct members (Issue #29)
PUB_SRC="struct Point { pub x: i64, y: i64, pub fn getx() -> i64 { return self.x; } }; fn main() -> i64 { let p: Point; p.x = 3; p.y = 4; return p.getx(); }"
assert_meta_section_contains "$PUB_SRC" "private" "^  pub x: i64;"
assert_meta_section_contains "$PUB_SRC" "private" "^  pub fn getx(self: &Point) -> i64;"
assert_meta_section_contains "$PUB_SRC" "private" "^  y: i64;"

# 6. Struct without pub members: no member gets pub
NOPUB_SRC="struct Point { x: i64, y: i64, fn getx() -> i64 { return self.x; } }; fn main() -> i64 { 0; }"
assert_meta_section_contains "$NOPUB_SRC" "private" "^  x: i64;"
assert_meta_section_contains "$NOPUB_SRC" "private" "^  y: i64;"
assert_meta_section_contains "$NOPUB_SRC" "private" "^  fn getx(self: &Point) -> i64;"

echo -e "${GREEN}All meta-writer tests passed successfully!${RESET}"
