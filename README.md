# BCH PQC Hybrid

**Post-Quantum Cryptography Hybrid Tool for Bitcoin Cash**

A proof-of-concept tool that combines classical secp256k1 keys with SPHINCS+ (SLH-DSA) post-quantum signatures for Bitcoin Cash. It generates realistic secp256k1 private keys while also producing linked SPHINCS+ material and alternative address formats using domain-separated SHAKE256 reductions.

---

## Motivation

This project explores whether it is possible to derive both classical and post-quantum keys from the same source material in a way that creates a strong cryptographic link between them. The goal is to experiment with hybrid key derivation that could support a gradual transition toward quantum resistance without immediately breaking existing Bitcoin Cash infrastructure.

Key areas of exploration include:
- Hybrid key derivation (classical & post-quantum)
- Deterministic linkage between EC keys and SPHINCS+ signatures
- Alternative address generation using domain-separated SHAKE256 reductions
- Structured control over SPHINCS+ output sizes

---

## Features

- Generates realistic secp256k1 private keys compatible with current BCH wallet formats
- Binds SPHINCS+ signatures to transaction data
- Produces two address formats from the same payload:
  - `standard_bch_checksum` — Uses double-SHA256 (standard BCH-style)
  - `pq_checksum` — Uses double-SHAKE256 (custom PQ-linked style)
- Outputs structured files using custom extensions:
  - `.kbch` — Main key material
  - `.bchkproof` — Transaction binding proof
- Includes a `sphincs_btc_pipeline_style` section for additional SPHINCS+-derived outputs

---

## Quick Start

### Build

```bash
git clone https://github.com/DigiMancer3D/sphincs_bch_hybrid.git
cd sphincs_bch_hybrid
make clean && make
```

### Testing (Recommended)

```bash
./test_bch_pqc.sh          # Default role 3
./test_bch_pqc.sh 5        # Use role 5
./test_bch_pqc.sh 0 my_tx.json
```

### Run the Main Program

```bash
./bch_pqc_hybrid_single --tx-data tx.json --role 3
```

See [USAGE.md](USAGE.md) for detailed instructions.

---

## Output Files

| File       | Extension    | Description                                      |
|------------|--------------|--------------------------------------------------|
| Keychain   | `.kbch`      | Main output containing EC keys and SPHINCS+ data |
| Proof      | `.bchkproof` | Transaction binding and metadata                 |

---

## Current Status

**Experimental / Proof of Concept**

This is research and reference code. It is **not** intended for production use or for managing real funds. While the tool produces realistic secp256k1 private keys that appear compatible with current wallets, they should be treated as experimental. The primary purpose of this project is to explore possibilities with SPHINCS+ (SLH-DSA) in the context of Bitcoin Cash.

---

