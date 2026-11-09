#!/bin/bash
# CI guard (issue #31): compiler output must not contain Japanese / CJK text.
#
# Re-runs the existing suites (test.sh, test_errors.sh, test_meta.sh) in a
# scratch directory where ./shiro is a thin wrapper around the real compiler.
# The wrapper records, for every invocation:
#   - the compiler's stdout and stderr (diagnostics)
#   - the generated assembly (-o file) and the meta file (-M)
# Afterwards the record is scanned for hiragana, katakana, kanji and other CJK
# characters. Runtime error messages are embedded in the assembly, so they are
# covered as well. No test cases are duplicated here.
#
# The dynamic check only sees code paths the suites exercise, so string
# literals in src/ are also scanned directly: a Japanese message added to a
# rarely hit CLI/fatal path would otherwise only be caught once a test
# happens to trigger it.
#
# Note: diagnostics echo the offending source line, so test inputs in
# test*.sh must also stay free of Japanese/CJK text, or this check will
# (correctly) fail on the echoed line.
#
# Usage: bash test_no_japanese.sh   (run `make` first)

ROOT="$(cd "$(dirname "$0")" && pwd)"
REAL="$ROOT/shiro"
if [ ! -x "$REAL" ]; then
    echo "shiro binary not found. Run 'make' first."
    exit 1
fi

# Hiragana, katakana, CJK ideographs (+ext A and B, compat), CJK punctuation,
# fullwidth forms
PATTERN='[\x{3000}-\x{30FF}\x{31F0}-\x{31FF}\x{3400}-\x{4DBF}\x{4E00}-\x{9FFF}\x{F900}-\x{FAFF}\x{FF00}-\x{FFEF}\x{20000}-\x{2A6DF}]'

# Sanity check: the pattern must match known Japanese text (U+3042, written
# here as octal escapes so this script stays pure ASCII). Without it, a grep
# lacking PCRE support would fail silently and this guard would pass anything.
if ! printf '\343\201\202' | LC_ALL=C.UTF-8 grep -qP "$PATTERN"; then
    echo -e "\e[31m[FAIL] grep -P does not support the Japanese-detection pattern; refusing to pass silently.\e[0m"
    exit 1
fi

# Static scan: CJK text inside a double-quoted string literal in src/.
# Only lines with a quote are checked, so a Japanese comment does not trip it.
echo "Scanning src/ string literals for Japanese/CJK text..."
if LC_ALL=C.UTF-8 grep -rnP "\"[^\"]*$PATTERN" "$ROOT/src/"; then
    echo -e "\e[31m[FAIL] Source files contain Japanese/CJK text in a string literal (see lines above).\e[0m"
    exit 1
fi

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
LOG="$WORK/compiler_output.log"
: > "$LOG"

cat > "$WORK/shiro" <<'WRAP'
#!/bin/bash
out=""; src=""; meta=0; prev=""
for a in "$@"; do
    case "$prev" in
        -o|--output) out="$a" ;;
    esac
    case "$a" in
        -M|--emit-meta) meta=1 ;;
        *.shiro) src="$a" ;;
    esac
    prev="$a"
done
tmp_out=$(mktemp)
tmp_err=$(mktemp)
"$SHIRO_REAL" "$@" > "$tmp_out" 2> "$tmp_err"
status=$?
{
    cat "$tmp_out" "$tmp_err"
    [ -n "$out" ] && [ "$out" != "-" ] && [ -f "$out" ] && cat "$out"
    [ "$meta" = 1 ] && [ -f "$src.meta" ] && cat "$src.meta"
} >> "$SHIRO_LOG"
cat "$tmp_out"
cat "$tmp_err" >&2
rm -f "$tmp_out" "$tmp_err"
exit $status
WRAP
chmod +x "$WORK/shiro"

echo "Running existing suites with compiler output capture..."
for t in test.sh test_errors.sh test_meta.sh; do
    cp "$ROOT/$t" "$WORK/$t"
    if ! (cd "$WORK" && SHIRO_REAL="$REAL" SHIRO_LOG="$LOG" bash "$t" > "$WORK/$t.out" 2>&1); then
        echo -e "\e[31m[FAIL] $t failed while capturing compiler output:\e[0m"
        tail -n 20 "$WORK/$t.out"
        exit 1
    fi
done

echo "Scanning $(wc -c < "$LOG") bytes of compiler output for Japanese/CJK text..."
if LC_ALL=C.UTF-8 grep -nP "$PATTERN" "$LOG"; then
    echo -e "\e[31m[FAIL] Compiler output contains Japanese/CJK text (see lines above).\e[0m"
    exit 1
fi
echo -e "\e[32mNo Japanese text found in compiler output.\e[0m"
