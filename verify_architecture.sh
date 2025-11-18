#!/bin/bash
# verify_architecture.sh
#
# Runs all architecture fitness functions from project root
# Exits with non-zero code if any fitness function fails
#
# Usage:
#   ./verify_architecture.sh
#   make verify  # (if added to Makefile)

set -e  # Exit on first failure

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo ""
echo -e "${BLUE}================================================================================${NC}"
echo -e "${BLUE}                    ARCHITECTURE VERIFICATION SUITE${NC}"
echo -e "${BLUE}================================================================================${NC}"
echo ""
echo "Project: Card Fifty-Two"
echo "Location: $PROJECT_ROOT"
echo "Python: $(python3 --version)"
echo ""

# Check if Python3 is available
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}❌ ERROR: python3 not found${NC}"
    echo "Please install Python 3 to run fitness functions"
    exit 1
fi

# Check if fitness functions directory exists
if [ ! -d "$PROJECT_ROOT/architecture/fitness_functions" ]; then
    echo -e "${RED}❌ ERROR: fitness_functions directory not found${NC}"
    echo "Expected: $PROJECT_ROOT/architecture/fitness_functions"
    exit 1
fi

# Run the main fitness function runner
echo -e "${YELLOW}Running all fitness functions...${NC}"
echo ""

cd "$PROJECT_ROOT"

if python3 architecture/fitness_functions/main.py; then
    echo ""
    echo -e "${GREEN}================================================================================${NC}"
    echo -e "${GREEN}✅ ALL ARCHITECTURE CHECKS PASSED${NC}"
    echo -e "${GREEN}================================================================================${NC}"
    echo ""
    exit 0
else
    echo ""
    echo -e "${RED}================================================================================${NC}"
    echo -e "${RED}❌ ARCHITECTURE VIOLATIONS DETECTED${NC}"
    echo -e "${RED}================================================================================${NC}"
    echo ""
    echo "Fix the violations above before committing."
    echo "See architecture/decision_records/ for context on each fitness function."
    echo ""
    exit 1
fi
