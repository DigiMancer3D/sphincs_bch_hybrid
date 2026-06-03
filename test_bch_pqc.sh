#!/bin/bash
set -e

echo "=== BCH PQC Hybrid Single - Real Data Test ==="

TXID="a1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d"
echo "Using real BCH txid: $TXID"
echo "Verify here: https://blockchair.com/bitcoin-cash/transaction/$TXID"

# === CLEANUP ===
echo ""
echo "→ Cleaning old test runs..."
rm -rf ./test_output
mkdir -p ./test_output
echo "   Old test data cleaned."

# === Handle arguments ===
# $1 = Role (default 3)
# $2 = Custom tx data file (optional)
ROLE=${1:-3}

if [ -n "$2" ] && [ -f "$2" ]; then
    TX_FILE="$2"
    echo "Using custom tx data file: $TX_FILE"
else
    # Create default test file
    cat > ./test_output/real_bch_tx.json << EOF
{
  "txid": "$TXID",
  "version": 2
}
EOF
    TX_FILE="./test_output/real_bch_tx.json"
    echo "Created default test file: $TX_FILE"
fi

# === Build using make ===
if [ ! -f ./bch_pqc_hybrid_single ] || [ bch_pqc_hybrid_single.c -nt ./bch_pqc_hybrid_single ]; then
    echo ""
    echo "→ Building with make..."
    make clean
    make
fi

echo ""
echo "Running hybrid tool with real BCH data (Role $ROLE)..."
./bch_pqc_hybrid_single --tx-data "$TX_FILE" --role "$ROLE"

echo ""
echo "=== Test Complete ==="
echo ""

echo "Generated files:"
ls -1 ./test_output/a10_d48d_role${ROLE}_*/kchain_role${ROLE}.kbch 2>/dev/null || true
ls -1 ./test_output/a10_d48d_role${ROLE}_*/tx_proof_role${ROLE}.bchkproof 2>/dev/null || true

echo ""
echo "=== sphincs_btc_pipeline_style section ==="
KCHAIN=$(ls ./test_output/a10_d48d_role${ROLE}_*/kchain_role${ROLE}.kbch 2>/dev/null | head -n 1)
if [ -f "$KCHAIN" ] && command -v jq &> /dev/null; then
    echo "File: $KCHAIN"
    jq '.sphincs_btc_pipeline_style' "$KCHAIN"
elif [ -f "$KCHAIN" ]; then
    echo "(jq not found)"
    grep -A 20 '"sphincs_btc_pipeline_style"' "$KCHAIN" || true
fi

echo ""
echo "=== Full test output directory ==="
ls -la ./test_output/
