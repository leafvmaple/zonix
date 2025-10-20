#!/bin/bash
# Zonix Swap System Test Runner

set -e

echo "========================================="
echo "  Zonix Swap System Test Runner"
echo "========================================="
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if we're in the right directory
if [ ! -f "Makefile" ]; then
    echo -e "${RED}Error: Must be run from Zonix root directory${NC}"
    exit 1
fi

# Step 1: Clean build
echo -e "${YELLOW}Step 1: Cleaning previous build...${NC}"
make clean > /dev/null 2>&1

# Step 2: Build
echo -e "${YELLOW}Step 2: Building Zonix with swap tests...${NC}"
if make > build.log 2>&1; then
    echo -e "${GREEN}[OK] Build successful${NC}"
else
    echo -e "${RED}[FAIL] Build failed${NC}"
    echo "Check build.log for details"
    exit 1
fi

# Step 3: Check if test file was compiled
if [ -f "obj/kern/mm/swap_test.o" ]; then
    echo -e "${GREEN}[OK] Swap test module compiled${NC}"
else
    echo -e "${YELLOW}[WARN] Warning: swap_test.o not found${NC}"
fi

# Step 4: Run in QEMU (manual mode)
echo ""
echo -e "${YELLOW}Step 3: Starting Zonix in QEMU...${NC}"
echo "================================================================"
echo "  To run swap tests, type: swaptest"
echo "  To exit QEMU: Ctrl+A, then X"
echo "================================================================"
echo ""
echo "Press Enter to start QEMU..."
read

make qemu

echo ""
echo -e "${GREEN}Test runner complete!${NC}"
