#define KEY_PRIVKEY		1
#define KEY_PUBKEY		2
#define DECRYPT			0
#define ENCRYPT			1
#define DICTIONARY_SIZE	60

#if !defined(PKCS5_SALT_LEN)
#define PKCS5_SALT_LEN	8
#define EVP_MAX_KEY_LENGTH	32
#define EVP_MAX_IV_LENGTH	16
#endif

#define PUBKEY_SKIP	48  // skip first 48 byte
#define PUBKEY_MOD	448 // public key is 550 bytes, 550 - PUBKEY_SKIP - sizeof(SEED_SIV)
struct seed_siv_st { unsigned char key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH]; };
typedef struct seed_siv_st SEED_SIV;

static DWORD key[4] = {0, 0, 0, 0};
unsigned long privateShieldPC(unsigned long x)
{
	unsigned long y,y1,y2,x1,x2,y11,y22;
	unsigned long outdata1,outdata2;
	outdata2 = x  & 0xffff0000;
	outdata2 = outdata2 >> 16;
	outdata1 = 0x0ffff & x;
	x1=outdata1;
	x2=outdata2;
	y1 = x1 ^ key[1];
	y11 = x2 ^ key[0];
	y1 = y1 + y11;
	if (y1 > 65535)
		y1 = y1 - 65536;
	y1 = y1 * 16;
	while (y1 > 65535)
		y1 = y1 - 65536;
	y1 = y1 % key[3];
	y = y1 * key[2];
	while (y > 4294967295)
		y = y - 4294967295-1;
	y11 = x1 + key[0];
	if (y11 > 65535)
		y11 = y11 - 65536;
	y22 = y11 % key[2];
	y11 = key[3] ^ x2;
	y2 = y22 * y11;
	while (y2 > 4294967295)
		y2 = y2 - 4294967295-1; 
	y = y ^ y2;
	return(y);
}
