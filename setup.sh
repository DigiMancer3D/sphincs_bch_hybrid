#!/bin/bash
set -e

echo "=== SPHINCS-BTC Pipeline Setup (bch_pqc_hybrid) ==="

if [ ! -d "ref-source" ]; then
    echo "→ Cloning official SPHINCS+ reference..."
    git clone https://github.com/sphincs/sphincsplus.git ref-source
fi

# Setup prog1 (preserves custom files)
echo "→ Setting up prog1..."
mkdir -p prog1
# Backup custom files
cp prog1/custom_params.h /tmp/ 2>/dev/null || true
cp prog1/main.c /tmp/ 2>/dev/null || true
cp prog1/params.h /tmp/ 2>/dev/null || true
cp prog1/Makefile /tmp/ 2>/dev/null || true

rm -rf prog1/*
cp -r ref-source/ref/* prog1/

# Restore custom files
mv /tmp/custom_params.h prog1/ 2>/dev/null || true
mv /tmp/main.c prog1/ 2>/dev/null || true
mv /tmp/params.h prog1/ 2>/dev/null || true
mv /tmp/Makefile prog1/ 2>/dev/null || true

# Setup prog2 (same logic)
echo "→ Setting up prog2..."
mkdir -p prog2
cp prog2/custom_params2.h /tmp/ 2>/dev/null || true
cp prog2/main.c /tmp/ 2>/dev/null || true
cp prog2/params.h /tmp/ 2>/dev/null || true
cp prog2/Makefile /tmp/ 2>/dev/null || true

rm -rf prog2/*
cp -r ref-source/ref/* prog2/

mv /tmp/custom_params2.h prog2/ 2>/dev/null || true
mv /tmp/main.c prog2/ 2>/dev/null || true
mv /tmp/params.h prog2/ 2>/dev/null || true
mv /tmp/Makefile prog2/ 2>/dev/null || true

echo "✅ Setup complete!"
