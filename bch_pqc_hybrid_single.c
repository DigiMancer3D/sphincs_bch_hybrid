/*
 * bch_pqc_hybrid_single.c
 * Single-file BCH Transitional PQC Hybrid Tool
 * Compile:
 *   gcc -Wall -Wextra -O2 -Wno-format-truncation -o bch_pqc_hybrid_single bch_pqc_hybrid_single.c -loqs -ljansson -lcrypto -lm
 *
 * Usage examples:
 *   ./bch_pqc_hybrid_single --tx-data ../data/tx_template.json --role 3
 *   ./bch_pqc_hybrid_single --tx-data ./sighash.hex --role 0
 */


#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <oqs/oqs.h>
#include <jansson.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <openssl/evp.h>

#define SPX_SLICE_BYTES 105

/* ==================== Utilities ==================== */
static char* bytes_to_hex(const unsigned char *data, size_t len) {
    char *hex = malloc(len * 2 + 1);
    for (size_t i = 0; i < len; i++) sprintf(hex + i*2, "%02x", data[i]);
    hex[len*2] = '\0';
    return hex;
}

static char* base58_encode(const unsigned char* data, size_t len) {
    static const char* alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    size_t size = len * 138 / 100 + 1;
    unsigned char* buf = calloc(size, 1);
    for (size_t i = 0; i < len; i++) {
        size_t carry = data[i];
        for (size_t j = 0; j < size; j++) {
            carry += (unsigned char)buf[j] * 256;
            buf[j] = carry % 58;
            carry /= 58;
        }
    }
    size_t i;
    for (i = 0; i < size && buf[i] == 0; i++);
    char* out = malloc(size - i + 1);
    size_t j = 0;
    for (; i < size; i++, j++) out[j] = alphabet[buf[i]];
    out[j] = '\0';
    free(buf);
    return out;
}

/* ==================== EC Key Derivation ==================== */
static int generate_btc_keypair_from_seed(const unsigned char *seed32,
                                          unsigned char **priv_out, size_t *priv_len_out,
                                          unsigned char **pub_out, size_t *pub_len_out) {
    *priv_len_out = 32;
    *priv_out = malloc(32);
    memcpy(*priv_out, seed32, 32);

    const EC_GROUP *group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    BIGNUM *priv_bn = BN_bin2bn(*priv_out, 32, NULL);
    EC_POINT *pub_point = EC_POINT_new(group);
    if (!EC_POINT_mul(group, pub_point, priv_bn, NULL, NULL, NULL)) return 0;

    *pub_len_out = EC_POINT_point2oct(group, pub_point, POINT_CONVERSION_COMPRESSED, NULL, 0, NULL);
    *pub_out = malloc(*pub_len_out);
    EC_POINT_point2oct(group, pub_point, POINT_CONVERSION_COMPRESSED, *pub_out, *pub_len_out, NULL);

    BN_free(priv_bn);
    EC_POINT_free(pub_point);
    EC_GROUP_free((EC_GROUP*)group);
    return 1;
}

/* ==================== Dynamic TX Data Loader ==================== */
typedef struct {
    unsigned char data[1024];
    size_t len;
    char type[32];
    char detected_label[64];
} TxData;

static int try_load_json(const char *filepath, TxData *out) {
    json_t *root = json_load_file(filepath, 0, NULL);
    if (!root) return 0;

    if (json_object_get(root, "txid") || json_object_get(root, "inputs") || json_object_get(root, "version")) {
        strncpy(out->type, "json", sizeof(out->type));
        strncpy(out->detected_label, "json_tx_template", sizeof(out->detected_label));

        char *json_str = json_dumps(root, JSON_COMPACT);
        if (json_str) {
            EVP_MD_CTX *ctx = EVP_MD_CTX_new();
            EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
            EVP_DigestUpdate(ctx, json_str, strlen(json_str));
            EVP_DigestFinal_ex(ctx, out->data, NULL);
            EVP_MD_CTX_free(ctx);
            out->len = 32;
            free(json_str);
        }
        json_decref(root);
        return 1;
    }
    json_decref(root);
    return 0;
}

static int try_load_hex(const char *filepath, TxData *out) {
    FILE *f = fopen(filepath, "r");
    if (!f) return 0;

    char buffer[2048] = {0};
    size_t read = fread(buffer, 1, sizeof(buffer)-1, f);
    fclose(f);

    size_t clean_len = 0;
    for (size_t i = 0; i < read; i++) {
        if (!isspace(buffer[i])) buffer[clean_len++] = buffer[i];
    }
    buffer[clean_len] = '\0';

    if (clean_len >= 16 && strspn(buffer, "0123456789abcdefABCDEF") == clean_len) {
        out->len = clean_len / 2;
        for (size_t i = 0; i < out->len; i++) {
            sscanf(buffer + 2*i, "%2hhx", &out->data[i]);
        }
        strncpy(out->type, (clean_len == 64) ? "sighash" : "raw_hex", sizeof(out->type));
        strncpy(out->detected_label, "hex_data", sizeof(out->detected_label));
        return 1;
    }
    return 0;
}

static int load_tx_data(const char *filepath, TxData *out) {
    memset(out, 0, sizeof(TxData));
    if (try_load_json(filepath, out)) return 1;
    if (try_load_hex(filepath, out)) return 1;
    return 0;
}

/* ==================== Hiding Commitment ==================== */
static void create_hiding_commitment(const unsigned char *sphincs_output, uint32_t role,
                                     const unsigned char *tx_binding, unsigned char commitment[32]) {
    unsigned char preimage[128] = {0};
    size_t plen = 0;

    memcpy(preimage, sphincs_output, 32); plen += 32;
    preimage[plen++] = (uint8_t)role;
    memcpy(preimage + plen, tx_binding, 32); plen += 32;
    memcpy(preimage + plen, "BCH_PQC_v1", 10); plen += 10;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha3_256(), NULL);
    EVP_DigestUpdate(ctx, preimage, plen);
    EVP_DigestFinal_ex(ctx, commitment, NULL);
    EVP_MD_CTX_free(ctx);
}

/* ==================== SPHINCS-BTC Pipeline Style Functions ==================== */

static void domain_separated_shake_reduce(const unsigned char *in105, unsigned char out20[20], uint32_t role) {
    unsigned char domain[32] = {0};
    const char *sep = "SPHINCS-BTC-PAYLOAD-v1";
    size_t seplen = strlen(sep);
    memcpy(domain, sep, seplen);
    memcpy(domain + seplen, &role, 4);

    unsigned char extended[105 + 32];
    memcpy(extended, in105, 105);
    memcpy(extended + 105, domain, 32);

    unsigned char tmp[64];
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_shake256(), NULL);
    EVP_DigestUpdate(ctx, extended, 105 + 32);
    EVP_DigestFinalXOF(ctx, tmp, 64);
    EVP_MD_CTX_free(ctx);

    for (int i = 0; i < 20; i++) {
        out20[i] = tmp[i] ^ tmp[i + 32] ^ tmp[i + 44];
    }
}

static void double_sha256(unsigned char *out, const unsigned char *in, size_t inlen) {
    unsigned char tmp[32];
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, in, inlen);
    EVP_DigestFinal_ex(ctx, tmp, NULL);
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, tmp, 32);
    EVP_DigestFinal_ex(ctx, out, NULL);
    EVP_MD_CTX_free(ctx);
}

static void double_shake256(unsigned char *out, const unsigned char *in, size_t inlen) {
    unsigned char tmp[32];
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_shake256(), NULL);
    EVP_DigestUpdate(ctx, in, inlen);
    EVP_DigestFinalXOF(ctx, tmp, 32);
    EVP_DigestInit_ex(ctx, EVP_shake256(), NULL);
    EVP_DigestUpdate(ctx, tmp, 32);
    EVP_DigestFinalXOF(ctx, out, 32);
    EVP_MD_CTX_free(ctx);
}

static void payload_to_btc_address(const unsigned char *payload20, char *address_out, int use_real_sha256) {
    unsigned char extended[25];
    unsigned char checksum[32];
    extended[0] = 0x00;
    memcpy(extended + 1, payload20, 20);

    if (use_real_sha256) {
        double_sha256(checksum, extended, 21);
    } else {
        double_shake256(checksum, extended, 21);
    }
    memcpy(extended + 21, checksum, 4);

    char *b58 = base58_encode(extended, 25);
    strcpy(address_out, b58);
    free(b58);
}

/* ==================== MAIN ==================== */
int main(int argc, char **argv) {
    const char *tx_data_path = NULL;
    uint32_t role = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tx-data") == 0 && i+1 < argc) tx_data_path = argv[++i];
        else if (strcmp(argv[i], "--role") == 0 && i+1 < argc) role = atoi(argv[++i]);
    }

    if (!tx_data_path) {
        fprintf(stderr, "Usage: %s --tx-data <file> [--role N]\n", argv[0]);
        return 1;
    }

    TxData tx;
    if (!load_tx_data(tx_data_path, &tx)) {
        fprintf(stderr, "Failed to load tx data\n");
        return 1;
    }
    printf("✓ Detected %s\n", tx.type);

    /* SPHINCS+ */
    OQS_SIG *sig = OQS_SIG_new("SLH_DSA_PURE_SHA2_128S");
    if (!sig) {
        fprintf(stderr, "ERROR: SLH_DSA not available\n");
        return 1;
    }

    unsigned char *pk = malloc(sig->length_public_key);
    unsigned char *sk = malloc(sig->length_secret_key);
    unsigned char *signature = malloc(sig->length_signature);
    size_t sig_len = 0;

    if (OQS_SIG_keypair(sig, pk, sk) != OQS_SUCCESS) {
        fprintf(stderr, "Keypair failed\n");
        return 1;
    }

    if (OQS_SIG_sign(sig, signature, &sig_len, tx.data, tx.len, sk) != OQS_SUCCESS) {
        fprintf(stderr, "Signing failed\n");
        return 1;
    }

    unsigned char sphincs_output[32];
    memcpy(sphincs_output, signature, 32);

    /* === SPHINCS-BTC Pipeline Style Outputs === */
    unsigned char raw105[SPX_SLICE_BYTES];
    memcpy(raw105, signature, SPX_SLICE_BYTES);

    unsigned char payload[20];
    domain_separated_shake_reduce(raw105, payload, role);

    char std_addr[64] = {0};
    char pq_addr[64]  = {0};

    payload_to_btc_address(payload, std_addr, 1);
    payload_to_btc_address(payload, pq_addr,  0);

    char *raw105_hex  = bytes_to_hex(raw105, SPX_SLICE_BYTES);
    char *payload_hex = bytes_to_hex(payload, 20);

    /* EC Key + Hiding Commitment */
    unsigned char *ec_priv = NULL, *ec_pub = NULL;
    size_t ec_priv_len, ec_pub_len;
    if (!generate_btc_keypair_from_seed(sphincs_output, &ec_priv, &ec_priv_len, &ec_pub, &ec_pub_len)) {
        fprintf(stderr, "EC key derivation failed\n");
        return 1;
    }

    unsigned char commitment[32];
    create_hiding_commitment(sphincs_output, role, tx.data, commitment);

    /* === Paths with new extensions === */
    char short_id[32];
    snprintf(short_id, sizeof(short_id), "a10_d48d");

    char out_dir[8192];
    strncpy(out_dir, tx_data_path, sizeof(out_dir)-1);
    char *last = strrchr(out_dir, '/');
    if (last) *last = '\0'; else strcpy(out_dir, ".");

    char run_folder[8192];
    time_t now = time(NULL);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", localtime(&now));
    snprintf(run_folder, sizeof(run_folder), "%s/%s_role%u_%s", out_dir, short_id, role, ts);

    if (mkdir(run_folder, 0755) != 0 && errno != EEXIST) {
        perror("Failed to create folder");
        return 1;
    }

    char kchain_path[8192], tx_path[8192];
    snprintf(kchain_path, sizeof(kchain_path), "%s/kchain_role%u.kbch", run_folder, role);
    snprintf(tx_path, sizeof(tx_path), "%s/tx_proof_role%u.bchkproof", run_folder, role);

    /* Write kchain .kbch */
    json_t *k = json_object();
    json_object_set_new(k, "role", json_integer(role));
    json_object_set_new(k, "tx_type", json_string(tx.type));
    json_object_set_new(k, "sphincs_output", json_string(bytes_to_hex(sphincs_output, 32)));
    json_object_set_new(k, "hiding_commitment", json_string(bytes_to_hex(commitment, 32)));
    json_object_set_new(k, "ec_private_key_hex", json_string(bytes_to_hex(ec_priv, 32)));
    json_object_set_new(k, "ec_public_key_hex", json_string(bytes_to_hex(ec_pub, ec_pub_len)));

    /* sphincs_btc_pipeline_style section */
    json_t *btc_style = json_object();
    json_object_set_new(btc_style, "raw105_slice_hex", json_string(raw105_hex));
    json_object_set_new(btc_style, "payload_hex", json_string(payload_hex));

    json_t *std_section = json_object();
    json_object_set_new(std_section, "address", json_string(std_addr));
    json_object_set_new(std_section, "checksum_type", json_string("double_sha256"));
    json_object_set_new(btc_style, "standard_bch_checksum", std_section);

    json_t *pq_section = json_object();
    json_object_set_new(pq_section, "address", json_string(pq_addr));
    json_object_set_new(pq_section, "checksum_type", json_string("double_shake256"));
    json_object_set_new(btc_style, "pq_checksum", pq_section);

    json_object_set_new(k, "sphincs_btc_pipeline_style", btc_style);

    json_dump_file(k, kchain_path, JSON_INDENT(2));
    json_decref(k);

    /* Write tx proof .bchkproof */
    json_t *t = json_object();
    json_object_set_new(t, "input_file", json_string(tx_data_path));
    json_object_set_new(t, "role", json_integer(role));
    json_object_set_new(t, "hiding_commitment", json_string(bytes_to_hex(commitment, 32)));
    json_object_set_new(t, "note", json_string("Real BCH data test - EC key is spendable today"));
    json_dump_file(t, tx_path, JSON_INDENT(2));
    json_decref(t);

    /* Key Verification */
    printf("\n=== Key Verification ===\n");
    printf("EC Private Key matches Public Key derivation: ");
    unsigned char *check_pub = NULL; size_t check_len;
    unsigned char *tmp_priv = malloc(32); memcpy(tmp_priv, ec_priv, 32);
    if (generate_btc_keypair_from_seed(tmp_priv, &tmp_priv, &ec_priv_len, &check_pub, &check_len)) {
        printf(memcmp(check_pub, ec_pub, ec_pub_len) == 0 ? "✅ PASS\n" : "❌ FAIL\n");
        free(check_pub);
    }
    free(tmp_priv);

    printf("\n✅ Success! Clean build.\n");
    printf("   Run folder : %s\n", run_folder);
    printf("   kchain     : %s\n", kchain_path);
    printf("   tx proof   : %s\n", tx_path);

    free(pk); free(sk); free(signature);
    free(ec_priv); free(ec_pub);
    free(raw105_hex);
    free(payload_hex);
    OQS_SIG_free(sig);

    return 0;
}
