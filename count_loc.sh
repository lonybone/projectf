#!/bin/bash
# Script to count lines of code in the projectf repository

echo "================================================"
echo "    Lines of Code in projectf Repository"
echo "================================================"
echo ""

# Count C source files
c_files=$(find ./src -name '*.c' 2>/dev/null | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')
echo "C Source Files (.c):        $c_files lines"

# Count C header files
h_files=$(find ./src -name '*.h' 2>/dev/null | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')
echo "C Header Files (.h):        $h_files lines"

# Total C code
echo "--------------------------------"
total_c=$((c_files + h_files))
echo "Total C Code (.c + .h):     $total_c lines"
echo ""

# Count other files
echo "Other Files:"
if [ -f ./README.md ]; then
    readme_lines=$(wc -l < ./README.md)
    echo "  README.md:                $readme_lines lines"
fi

if [ -f ./src/Makefile ]; then
    makefile_lines=$(wc -l < ./src/Makefile)
    echo "  Makefile:                 $makefile_lines lines"
fi

if [ -f ./src/test.txt ]; then
    test_lines=$(wc -l < ./src/test.txt)
    echo "  test.txt (test code):     $test_lines lines"
fi

echo ""
echo "--------------------------------"

# Total all files (excluding .git)
total_all=$(find . -type f -not -path './.git/*' -not -name 'count_loc.sh' 2>/dev/null | xargs wc -l 2>/dev/null | tail -1 | awk '{print $1}')
echo "Total (all files):          $total_all lines"
echo "================================================"
