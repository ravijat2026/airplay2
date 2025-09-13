#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <stdint.h>
#include <stddef.h>

// SHA-1 functions
int sha1_hash(const uint8_t *data, size_t length, uint8_t *hash);
int sha1_hmac(const uint8_t *key, size_t key_length, 
              const uint8_t *data, size_t data_length, uint8_t *hmac);

// AES functions
int aes_encrypt(const uint8_t *key, const uint8_t *iv, 
                const uint8_t *plaintext, size_t plaintext_length,
                uint8_t *ciphertext, size_t *ciphertext_length);
int aes_decrypt(const uint8_t *key, const uint8_t *iv,
                const uint8_t *ciphertext, size_t ciphertext_length,
                uint8_t *plaintext, size_t *plaintext_length);

// Base64 functions
int base64_encode(const uint8_t *data, size_t length, char *encoded);
int base64_decode(const char *encoded, uint8_t *data, size_t *length);

// Random number generation
int generate_random_bytes(uint8_t *buffer, size_t length);

// AirPlay specific crypto functions
int airplay_generate_pairing_key(uint8_t *key, size_t key_size);
int airplay_verify_pairing(const uint8_t *challenge, size_t challenge_length,
                          const uint8_t *response, size_t response_length,
                          const uint8_t *key, size_t key_length);

#endif // CRYPTO_UTILS_H
