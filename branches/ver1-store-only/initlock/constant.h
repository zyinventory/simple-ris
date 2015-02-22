#pragma once

#define KEY_PRIVKEY		1
#define KEY_PUBKEY		2
#define ENCRYPT			1

#if !defined(PKCS5_SALT_LEN)
#define PKCS5_SALT_LEN	8
#define EVP_MAX_KEY_LENGTH	32
#define EVP_MAX_IV_LENGTH	16
#endif

static char passwd[] = "wlt2911@^$";
