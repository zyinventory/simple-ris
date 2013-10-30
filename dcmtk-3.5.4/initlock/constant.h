#define KEY_PRIVKEY		1
#define KEY_PUBKEY		2
#define DECRYPT			0
#define ENCRYPT			1
#define PUBKEY_OFFSET	48  // no special meaning

#if !defined(PKCS5_SALT_LEN)
#define PKCS5_SALT_LEN	8
#define EVP_MAX_KEY_LENGTH	32
#define EVP_MAX_IV_LENGTH	16
#endif

struct seed_siv_st
{
	unsigned char salt[PKCS5_SALT_LEN], key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH];
};
typedef struct seed_siv_st SEED_SIV;
