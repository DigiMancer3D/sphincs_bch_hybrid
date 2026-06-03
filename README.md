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

## Prerequisites 

Before building, you need the following development libraries installed:

| Library       | Purpose                          | Debian/Ubuntu package      | Fedora/RHEL package       | macOS (Homebrew) |
|---------------|----------------------------------|----------------------------|---------------------------|------------------|
| **liboqs**    | SLH-DSA / SPHINCS+ (post-quantum) | `liboqs-dev`              | `liboqs-devel`           | `liboqs`        |
| **libjansson**| JSON parsing & generation       | `libjansson-dev`          | `jansson-devel`          | `jansson`       |
| **OpenSSL**   | EC (secp256k1), SHA, etc.       | `libssl-dev`              | `openssl-devel`          | `openssl`       |

**Quick install commands:**

```bash
# Debian / Ubuntu
sudo apt update
sudo apt install liboqs-dev libjansson-dev libssl-dev

# Fedora / RHEL / Rocky
sudo dnf install liboqs-devel jansson-devel openssl-devel

# macOS
brew install liboqs jansson openssl
```

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

## Setup(s) [various]

**Easy one-command setup (recommended):**

```bash
make setup-progs
```

This automatically:
1. Creates the initial folder structure.
2. Downloads the **official SPHINCS+ (SLH-DSA) reference implementation twice** — once into `prog1/` and once into `prog2/`.
3. Leaves you ready to overlay any "changed files" from this git repo on top if you are patching the reference code.

After running it you can do:

```bash
make prog1
make prog2
make all-progs
```

**Intended original workflow:**
1. Create main working folder.
2. Download official SPHINCS+ ref **twice** into `prog1/` and `prog2/`.
3. Overlay the changed files from this git repo.
4. Build with the special Makefile.

**Current status & recommendation:**  
The main program (`bch_pqc_hybrid_single`) handles SLH-DSA/SPHINCS+ (cleaner & easier for users).  

The Makefile is written so that forgetting the prog setup **never breaks** the normal build.


### Manual Setup (Alternative [adapted from the older sphincs-btc-pipeline project])

If you prefer the classic step-by-step manual process (as documented in the predecessor project https://github.com/DigiMancer3D/sphincs-btc-pipeline), here is the equivalent for this BCH hybrid version:

```bash
# 1. Create / enter your main working folder
mkdir -p sphincs_bch_hybrid
cd sphincs_bch_hybrid

# 2. (Optional but recommended) Install the easy dependencies first
make install-deps   # or manually install liboqs-dev, libjansson-dev, libssl-dev

# 3. Get the official SPHINCS+ (SLH-DSA) reference implementation
git clone https://github.com/sphincs/sphincsplus.git ref-source

# 4. Create the two working folders (this is the "download twice" part)
mkdir -p prog1 prog2

# 5. Copy the reference implementation into BOTH folders
cp -r ref-source/ref/* prog1/
cp -r ref-source/ref/* prog2/

# 6. (Optional) Overlay any changed/patched files from this hybrid repo
#    into prog1/ and/or prog2/ if you are modifying the reference code itself.
#    The main hybrid program lives in the root as bch_pqc_hybrid_single.c
#    and uses liboqs, so no changes to prog1/prog2 are required for normal use.

# 7. Build the main hybrid program (recommended path)
make clean && make

# 8. (Legacy path) If you want to build inside the prog folders
cd prog1
make clean && make   # only if prog1/ has its own Makefile

cd ../prog2
make clean && make
```

**Notes for this BCH version:**
- The main program (`bch_pqc_hybrid_single`) does **not** require anything inside `prog1/` or `prog2/`. It links against the system library.
- The `prog1/` and `prog2/` folders are only needed if you want the raw reference sources for inspection, custom parameter experiments, or applying patches.
- The one-command `make setup-progs` does steps 3-5 automatically for you.
- This manual process is provided for users coming from the older `sphincs-btc-pipeline` project who are used to this workflow.

---



## Build Details

The project uses a custom `Makefile` with the following main targets:

| Target          | Description |
|-----------------|-------------|
| `make` / `make all` | Build `bch_pqc_hybrid_single` (uses liboqs) — recommended for normal use |
| `make clean`        | Remove the binary |
| `make setup-progs`  | **Download official SPHINCS+ ref into prog1/ + prog2/** (the "twice" setup) |
| `make prog1`        | Build inside prog1/ (auto-suggests setup-progs if missing) |
| `make prog2`        | Build inside prog2/ (auto-suggests setup-progs if missing) |
| `make all-progs`    | Build both legacy prog dirs |
| `make install-deps` | Show dependency installation commands (does not install) |

---

## Current Status

**Experimental / Proof of Concept**

This is research and reference code. It is **not** intended for production use or for managing real funds. While the tool produces realistic secp256k1 private keys that appear compatible with current wallets, they should be treated as experimental. The primary purpose of this project is to explore possibilities with SPHINCS+ (SLH-DSA) in the context of Bitcoin Cash.

---

