// https://lucidar.me/en/dev-c-cpp/sha-256-in-c-cpp/

#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>
#include <stdint.h>

#define SHA256_HEX_SIZE (64 + 1)
#define SHA256_BYTES_SIZE 32

/*
 * Compute the SHA-256 checksum of a memory region given a pointer and
 * the size of that memory region.
 * The output is a hexadecimal string of 65 characters.
 * The last character will be the null-character.
 */
void sha256_hex(const void *src, size_t n_bytes, char *dst_hex65);
void sha256_bytes(const void *src, size_t n_bytes, void *dst_bytes32);

#endif
