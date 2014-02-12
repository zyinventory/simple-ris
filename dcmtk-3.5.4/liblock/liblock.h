#pragma once

#define KEY_SIZE		4096
#define DICTIONARY_SIZE	56
#define AES_OFFSET		16
#define DECRYPT			0

#if !defined(PKCS5_SALT_LEN)
#define PKCS5_SALT_LEN	8
#define EVP_MAX_KEY_LENGTH	32
#define EVP_MAX_IV_LENGTH	16
#endif

struct seed_siv_st { unsigned char key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH]; };
typedef struct seed_siv_st SEED_SIV;
#define PUBKEY_SKIP	48  /* skip first 48 byte */
#define PUBKEY_MOD	448 /* public key is 550 bytes, 550 - PUBKEY_SKIP - sizeof(SEED_SIV) */
static char init_lock_passwd[] = "abcdefgh", init_rw_passwd[] = "12345678";

#ifdef __cplusplus
extern "C"
{
#endif
	int fillSeedSIV(void *siv, size_t sivSize, void *content, size_t contentLength, size_t start);
	void MD5_digest(void *data, size_t dataLength, unsigned char *md);
	char *md5crypt(const char *passwd, const char *magic, const char *salt);
	int getLockNumber(const char *filter, int isDirectory, char *lockname);
	void mkpasswd(const char *base64, unsigned int salt, char *lock_passwd);
	int loadPublicKeyContentRW(const char* publicKey, SEED_SIV *siv, unsigned int lockNumber, char **dataptr, char *gen_rw_passwd);
	int loadPublicKeyContent(const char* publicKey, SEED_SIV *siv, unsigned int lockNumber, char *gen_lock_passwd, char *gen_rw_passwd);
	int rsaVerify(const unsigned char *inBuf, size_t inLen, unsigned char *outBuf, const char *keyfile);
	int aes256cbc_dec(const unsigned char *inBuf, size_t inLen, unsigned char *outBuf, unsigned char *key, unsigned char* iv);
	int invalidLock(const char *licenseRSAEnc, const char *rsaPublicKey, SEED_SIV *sivptr);
	int currentCount(char *passwd);
	int decreaseCount(char *passwd);
	int increaseCount(char *passwd, int charge);
#ifdef __cplusplus
}
#endif
