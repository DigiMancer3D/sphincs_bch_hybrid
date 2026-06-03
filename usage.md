
# BCH PQC Hybrid Tool - Usage Guide

This document explains how to build and use the `bch_pqc_hybrid_single` program.

---

## 1. Building (Make)

Always build using the provided `Makefile` for consistent compiler flags.

```bash
cd ~/bch_pqc_hybrid

make clean && make
```

This produces the binary: `bch_pqc_hybrid_single`

---

## 2. Testing (Recommended First Step)

Use the `test_bch_pqc.sh` script for safe testing.

### Basic Usage

```bash
./test_bch_pqc.sh
```

### With a Specific Role

```bash
./test_bch_pqc.sh 5
./test_bch_pqc.sh 0
./test_bch_pqc.sh 9
```

### With a Custom Transaction Data File

```bash
./test_bch_pqc.sh 3 path/to/your_tx.json
./test_bch_pqc.sh 7 my_sighash.hex
```

**Recommended Workflow:**
1. Always run the test script first with your desired role and data.
2. Verify the output (especially the `sphincs_btc_pipeline_style` section).
3. Only then use the main program for important work.

---

## 3. Using the Main Program

### Basic Syntax

```bash
./bch_pqc_hybrid_single --tx-data <file> --role <N>
```

### Examples

```bash
# Default role 3 with test data
./bch_pqc_hybrid_single --tx-data ./test_output/real_bch_tx.json --role 3

# Specific role
./bch_pqc_hybrid_single --tx-data my_transaction.json --role 5

# Using a raw hex sighash
./bch_pqc_hybrid_single --tx-data sighash.hex --role 0
```

### Output Files

The program creates two files inside a timestamped folder:

| File | Extension     | Description                              |
|------|---------------|------------------------------------------|
| kchain | `.kbch`       | Main key material + `sphincs_btc_pipeline_style` |
| tx proof | `.bchkproof` | Transaction binding proof                |

Example output paths:
- `kchain_role3.kbch`
- `tx_proof_role3.bchkproof`

---

## Recommended Workflow: Test → Main Use

We strongly recommend the following order:

```bash
# 1. Build
make clean && make

# 2. Test first (highly recommended)
./test_bch_pqc.sh 3                    # Default test
./test_bch_pqc.sh 5 my_tx.json         # Custom role + file

# 3. Only after testing, use the main program
./bch_pqc_hybrid_single --tx-data my_tx.json --role 5
```

This "Test First" approach helps catch issues early and ensures you understand the output before using it with real data.

---

## File Extensions

| Extension     | Meaning                              | Purpose                     |
|---------------|--------------------------------------|-----------------------------|
| `.kbch`       | Kinda-different BCH data             | Main keychain output        |
| `.bchkproof`  | BCH Kinda-different Proof            | Transaction proof output    |

These extensions make it easy to distinguish these hybrid PQC files from normal JSON files.
