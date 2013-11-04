#pragma once

#if !defined(PKCS5_SALT_LEN)
#define PKCS5_SALT_LEN	8
#define EVP_MAX_KEY_LENGTH	32
#define EVP_MAX_IV_LENGTH	16
#endif

struct seed_siv_st { unsigned char key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH]; };
typedef struct seed_siv_st SEED_SIV;
#define PUBKEY_SKIP	48  // skip first 48 byte
#define PUBKEY_MOD	448 // public key is 550 bytes, 550 - PUBKEY_SKIP - sizeof(SEED_SIV)

extern "C"
{
	int fillSeedSIV(void *siv, size_t sivSize, void *content, size_t contentLength, size_t start);
	void MD5_digest(void *data, size_t dataLength, unsigned char *md);
	char *md5crypt(const char *passwd, const char *magic, const char *salt);
	unsigned int getLockNumber(const char *filter, const char *regxPattern, int isDirectory, char *lockname);
	void mkpasswd(const char *base64, unsigned int salt, char *lock_passwd);
	int loadPublicKeyContent(const char* publicKey, SEED_SIV *siv, unsigned int lockNumber, char *gen_passwd);
}
