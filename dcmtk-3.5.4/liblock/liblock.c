#include <openssl/err.h>
#include <openssl/ossl_typ.h>
#include <openssl/ui.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/md5.h>

int fillSeedSIV(void *siv, size_t sivSize, void *content, size_t contentLength, size_t start)
{
	int read = 0, i;
	void *skip = NULL;
	BIO *b64 = NULL, *bio = BIO_new_mem_buf(content, contentLength);

	if(bio == NULL) goto fill_end;
	b64 = BIO_new(BIO_f_base64());
	bio = BIO_push(b64, bio);
	if(start != 0)
	{
		skip = malloc(start);
		read = BIO_read(bio, skip, start);
		if(read != start)
		{
			read = 0;
			goto fill_end;
		}
	}
	read = BIO_read(bio, siv, sivSize);
	for(i = 0; i < read; ++i)
	{
		if((((unsigned char*)siv)[i] & 0xf0) == 0)
			((unsigned char*)siv)[i] |= 0x30;
	}
fill_end:
	if(skip != NULL) free(skip);
	if(bio != NULL) BIO_free_all(bio);
	return read;
}

void MD5_digest(void *data, size_t dataLength, unsigned char *md)
{
	MD5_CTX ctx;
	MD5_Init(&ctx);
    MD5_Update(&ctx,data, dataLength);
    MD5_Final(md,&ctx);
}
