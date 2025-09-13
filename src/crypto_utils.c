#include "crypto_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>

// SHA-1 functions
int sha1_hash(const uint8_t *data, size_t length, uint8_t *hash) {
    if (!data || !hash) {
        return -1;
    }
    
    SHA1(data, length, hash);
    return 0;
}

int sha1_hmac(const uint8_t *key, size_t key_length, 
              const uint8_t *data, size_t data_length, uint8_t *hmac) {
    if (!key || !data || !hmac) {
        return -1;
    }
    
    unsigned int hmac_length = SHA_DIGEST_LENGTH;
    HMAC(EVP_sha1(), key, key_length, data, data_length, hmac, &hmac_length);
    return 0;
}

// AES functions
int aes_encrypt(const uint8_t *key, const uint8_t *iv, 
                const uint8_t *plaintext, size_t plaintext_length,
                uint8_t *ciphertext, size_t *ciphertext_length) {
    if (!key || !iv || !plaintext || !ciphertext || !ciphertext_length) {
        return -1;
    }
    
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return -1;
    }
    
    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    int len;
    if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_length) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    *ciphertext_length = len;
    
    if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    *ciphertext_length += len;
    EVP_CIPHER_CTX_free(ctx);
    return 0;
}

int aes_decrypt(const uint8_t *key, const uint8_t *iv,
                const uint8_t *ciphertext, size_t ciphertext_length,
                uint8_t *plaintext, size_t *plaintext_length) {
    if (!key || !iv || !ciphertext || !plaintext || !plaintext_length) {
        return -1;
    }
    
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return -1;
    }
    
    if (EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    int len;
    if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_length) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    *plaintext_length = len;
    
    if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    *plaintext_length += len;
    EVP_CIPHER_CTX_free(ctx);
    return 0;
}

// Base64 encoding table
static const char base64_chars[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int base64_encode(const uint8_t *data, size_t length, char *encoded) {
    if (!data || !encoded) {
        return -1;
    }
    
    size_t encoded_length = ((length + 2) / 3) * 4;
    size_t i, j;
    
    for (i = 0, j = 0; i < length;) {
        uint32_t a = i < length ? data[i++] : 0;
        uint32_t b = i < length ? data[i++] : 0;
        uint32_t c = i < length ? data[i++] : 0;
        
        uint32_t triple = (a << 16) + (b << 8) + c;
        
        encoded[j++] = base64_chars[(triple >> 18) & 0x3F];
        encoded[j++] = base64_chars[(triple >> 12) & 0x3F];
        encoded[j++] = base64_chars[(triple >> 6) & 0x3F];
        encoded[j++] = base64_chars[triple & 0x3F];
    }
    
    // Add padding
    for (i = 0; i < (3 - length % 3) % 3; i++) {
        encoded[encoded_length - 1 - i] = '=';
    }
    
    encoded[encoded_length] = '\0';
    return 0;
}

int base64_decode(const char *encoded, uint8_t *data, size_t *length) {
    if (!encoded || !data || !length) {
        return -1;
    }
    
    size_t encoded_length = strlen(encoded);
    if (encoded_length % 4 != 0) {
        return -1;
    }
    
    size_t decoded_length = (encoded_length * 3) / 4;
    if (encoded[encoded_length - 1] == '=') {
        decoded_length--;
    }
    if (encoded[encoded_length - 2] == '=') {
        decoded_length--;
    }
    
    if (*length < decoded_length) {
        return -1;
    }
    
    *length = decoded_length;
    
    for (size_t i = 0, j = 0; i < encoded_length;) {
        uint32_t a = encoded[i] == '=' ? 0 & i++ : strchr(base64_chars, encoded[i++]) - base64_chars;
        uint32_t b = encoded[i] == '=' ? 0 & i++ : strchr(base64_chars, encoded[i++]) - base64_chars;
        uint32_t c = encoded[i] == '=' ? 0 & i++ : strchr(base64_chars, encoded[i++]) - base64_chars;
        uint32_t d = encoded[i] == '=' ? 0 & i++ : strchr(base64_chars, encoded[i++]) - base64_chars;
        
        uint32_t triple = (a << 18) + (b << 12) + (c << 6) + d;
        
        if (j < decoded_length) data[j++] = (triple >> 16) & 0xFF;
        if (j < decoded_length) data[j++] = (triple >> 8) & 0xFF;
        if (j < decoded_length) data[j++] = triple & 0xFF;
    }
    
    return 0;
}

int generate_random_bytes(uint8_t *buffer, size_t length) {
    if (!buffer || length == 0) {
        return -1;
    }
    
    if (RAND_bytes(buffer, length) != 1) {
        return -1;
    }
    
    return 0;
}

int airplay_generate_pairing_key(uint8_t *key, size_t key_size) {
    if (!key || key_size < 16) {
        return -1;
    }
    
    return generate_random_bytes(key, key_size);
}

int airplay_verify_pairing(const uint8_t *challenge, size_t challenge_length,
                          const uint8_t *response, size_t response_length,
                          const uint8_t *key, size_t key_length) {
    if (!challenge || !response || !key) {
        return -1;
    }
    
    uint8_t expected_hmac[SHA_DIGEST_LENGTH];
    if (sha1_hmac(key, key_length, challenge, challenge_length, expected_hmac) != 0) {
        return -1;
    }
    
    if (response_length != SHA_DIGEST_LENGTH) {
        return -1;
    }
    
    return memcmp(expected_hmac, response, SHA_DIGEST_LENGTH) == 0 ? 0 : -1;
}
