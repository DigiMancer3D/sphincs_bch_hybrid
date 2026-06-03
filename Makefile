CC = gcc
CFLAGS = -Wall -Wextra -O2 -Wno-format-truncation
LIBS = -loqs -ljansson -lcrypto -lm
TARGET = bch_pqc_hybrid_single

.PHONY: all clean prog1 prog2 all-progs install-deps setup-progs

all: $(TARGET)

$(TARGET): bch_pqc_hybrid_single.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f $(TARGET)

# --- Legacy / Optional targets for prog1/ and prog2/ ---
# These were intended for having local copies of the official SPHINCS+ (SLH-DSA)
# reference implementation so you can apply "changed files" / patches on top.
#
# The official way clients were supposed to set this up:
#   1. Create the main folder
#   2. Download the official SPHINCS+ ref **twice** (into prog1/ and prog2/)
#   3. Overlay the changed files from this git repo
#
# In the **current** version of this project the main program uses the liboqs
# library instead.
#
# NEW EASY WAY: Run `make setup-progs` (see target below). It will automatically
# download the official reference into both prog1/ and prog2/ for you.
#.

prog1:
	@if [ -d prog1 ]; then \
		echo "==> Building in prog1/..."; \
		$(MAKE) -C prog1 clean || true; \
		$(MAKE) -C prog1; \
	else \
		echo "==> prog1/ not found."; \
		echo "    Run 'make setup-progs' to automatically download the official SPHINCS+ ref into prog1/ and prog2/."; \
		echo "    (This is the legacy/local-ref path. The main program uses liboqs and works without it.)"; \
	fi

prog2:
	@if [ -d prog2 ]; then \
		echo "==> Building in prog2/..."; \
		$(MAKE) -C prog2 clean || true; \
		$(MAKE) -C prog2; \
	else \
		echo "==> prog2/ not found."; \
		echo "    Run 'make setup-progs' to automatically download the official SPHINCS+ ref into prog1/ and prog2/."; \
		echo "    (This is the legacy/local-ref path. The main program uses liboqs and works without it.)"; \
	fi

all-progs: prog1 prog2

# Optional helper to remind about dependencies (does not auto-install)
install-deps:
	@echo "This project requires the following development libraries:"
	@echo "  - liboqs (for SLH-DSA / SPHINCS+)"
	@echo "  - libjansson (JSON)"
	@echo "  - libssl / libcrypto (OpenSSL)"
	@echo ""
	@echo "On Debian/Ubuntu:"
	@echo "  sudo apt update"
	@echo "  sudo apt install liboqs-dev libjansson-dev libssl-dev"
	@echo ""
	@echo "On Fedora/RHEL:"
	@echo "  sudo dnf install liboqs-devel jansson-devel openssl-devel"
	@echo ""
	@echo "On macOS (Homebrew):"
	@echo "  brew install liboqs jansson openssl"
	@echo ""
	@echo "After installing, run: make clean && make"

# === NEW: One-command setup for the legacy prog1/ + prog2/ workflow ===
# This downloads the official SPHINCS+ (SLH-DSA) reference implementation
# into BOTH prog1/ and prog2/ 
#
# Usage:  make setup-progs
# Then:   make prog1   or   make prog2   or   make all-progs
setup-progs:
	@echo "=== Downloading official SPHINCS+ (SLH-DSA) reference into prog1/ and prog2/ ==="
	@mkdir -p prog1 prog2
	@if [ ! -d prog1/.git ]; then \
		echo "Cloning https://github.com/sphincs/sphincsplus into prog1/..."; \
		git clone --depth 1 https://github.com/sphincs/sphincsplus.git prog1; \
	else \
		echo "prog1/ already has a git repo — skipping clone."; \
	fi
	@if [ ! -d prog2/.git ]; then \
		echo "Creating second copy in prog2/..."; \
		rm -rf prog2/* prog2/.* 2>/dev/null || true; \
		cp -a prog1/. prog2/; \
	else \
		echo "prog2/ already has a git repo — skipping copy."; \
	fi
	@echo ""
	@echo "Done! prog1/ and prog2/ now contain the official SPHINCS+ reference."
	@echo "You can now place any changed/patched files from this repo on top of them."
	@echo "Run 'make prog1' or 'make prog2' to build inside those directories (if they have Makefiles)."
	@echo ""
	@echo "Note: The main 'bch_pqc_hybrid_single' program still uses liboqs (recommended)."
	@echo "      These prog dirs are only needed if you want to work with the raw reference sources."
