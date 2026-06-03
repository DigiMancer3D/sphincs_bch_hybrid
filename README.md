
# BCH PQC Hybrid

**Post-Quantum Cryptography Hybrid Tool for Bitcoin Cash**

A proof-of-concept tool that combines classical secp256k1 keys with SPHINCS+ (SLH-DSA) post-quantum signatures for Bitcoin Cash. It generates realistic-spendable EC keys while also producing linked SPHINCS+ material and alternative address formats using double-SHAKE256 reductions.

---

## Motivation

If we can get a set of keys both traditional and PQC from the same starting material then we should be able to sign to priv in a manner that would allow for the SPHINCS+ output to be the private key of the paired address. 

This project experiments with:
- Hybrid key derivation (classical & post-quantum)
- Deterministic linkage between EC keys and SPHINCS+ signatures 
- Alternative address generation using domain-separated SHAKE256 reductions
- Structured use of SPHINCS+ to get lowered size outputs, allowing control of SPHINCS+ sized outputs that can be returned to full-size if an official PQC upgrade was to occur with SPHINCS+ in mind.
- Explore practical ways to sign toward quantum resistance without breaking existing infrastructure.

---

## Features

- Generates realistic spendable secp256k1 private keys
- Binds SPHINCS+ signatures to transaction data
- Produces two address styles from the same payload:
  - `standard_bch_checksum` — Double-SHA256 (normal BCH-valid style)
  - `pq_checksum` — Double-SHAKE256 (custom PQ-linked style)
- Outputs clean, labeled JSON files with custom extensions (the json extensions just helps identify which are proofs and keys for this demostration):
  - `.kbch` — Main key material "Kinda-different" BCH
  - `.bchkproof` — Transaction "Kinda-binding" proof
- Includes a `sphincs_btc_pipeline_style` section for additional contextual output styles that are possible with SPHINCS+

---

## Quick Start

### Build

```bash
git clone https://github.com/DigiMancer3D/sphincs_bch_hybrid.git
cd bch-pqc-hybrid
make clean && make
```

### Test First (Recommended)

```bash
./test_bch_pqc.sh          # Default role 3
./test_bch_pqc.sh 5        # Role 5
./test_bch_pqc.sh 0 my_tx.json
```

### Run the Main Program

```bash
./bch_pqc_hybrid_single --tx-data tx.json --role 3
```

See [USAGE.md](USAGE.md) for detailed instructions.

---

## Output Files

| File | Extension   | Description |
|------|-------------|-----------|
| Keychain | `.kbch`     | Main output containing EC keys + `sphincs_btc_pipeline_style` |
| Proof    | `.bchkproof`| Transaction binding and metadata |

---

## Current Status

**Experimental / Proof of Concept**

This is conceptual research refrence code. This is **not** intended for production use nor use with potential funds, keys, live-transactions; addresses. Although this produces realistic looking keys, they may not be setup properly. The idea in the repo only should be used to show possibilities with SPHINCS+ (SLH-DSA).

---

