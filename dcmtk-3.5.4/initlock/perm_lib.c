#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>

int PACS_PEM_ASN1_write_bio(i2d_of_void *i2d, const char *name, BIO *bp,
		       char *x, const EVP_CIPHER *enc, unsigned char *kstr,
		       int klen, pem_password_cb *callback, void *u)
{
	EVP_CIPHER_CTX ctx;
	int dsize=0,i,j,ret=0;
	unsigned char *p,*data=NULL;
	const char *objstr=NULL;
	char buf[PEM_BUFSIZE];
	unsigned char key[EVP_MAX_KEY_LENGTH];
	unsigned char iv[EVP_MAX_IV_LENGTH] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	
	if (enc != NULL)
	{
		objstr=OBJ_nid2sn(EVP_CIPHER_nid(enc));
		if (objstr == NULL)
		{
			PEMerr(PEM_F_PEM_ASN1_WRITE_BIO,PEM_R_UNSUPPORTED_CIPHER);
			goto err;
		}
	}

	if ((dsize=i2d(x,NULL)) < 0)
	{
		PEMerr(PEM_F_PEM_ASN1_WRITE_BIO,ERR_R_ASN1_LIB);
		dsize=0;
		goto err;
	}
	/* dzise + 8 bytes are needed */
	/* actually it needs the cipher block size extra... */
	data=(unsigned char *)OPENSSL_malloc((unsigned int)dsize+20);
	if (data == NULL)
	{
		PEMerr(PEM_F_PEM_ASN1_WRITE_BIO,ERR_R_MALLOC_FAILURE);
		goto err;
	}
	p=data;
	i=i2d(x,&p);

	if (enc != NULL)
	{
		if (kstr == NULL)
		{
			if (callback == NULL)
				klen=PEM_def_callback(buf,PEM_BUFSIZE,1,u);
			else
				klen=(*callback)(buf,PEM_BUFSIZE,1,u);
			if (klen <= 0)
			{
				PEMerr(PEM_F_PEM_ASN1_WRITE_BIO,PEM_R_READ_KEY);
				goto err;
			}
#ifdef CHARSET_EBCDIC
			/* Convert the pass phrase from EBCDIC */
			ebcdic2ascii(buf, buf, klen);
#endif
			kstr=(unsigned char *)buf;
		}
		RAND_add(data,i,0);/* put in the RSA key. */
		OPENSSL_assert(enc->iv_len <= (int)sizeof(iv));

		/* Generate a salt */
		//if (RAND_pseudo_bytes(iv,enc->iv_len) < 0)
		//	goto err;

		/* The 'iv' is used as the iv and as a salt.  It is
		 * NOT taken from the BytesToKey function */
		EVP_BytesToKey(enc,EVP_md5(),iv,kstr,klen,1,key,NULL);

		if (kstr == (unsigned char *)buf) OPENSSL_cleanse(buf,PEM_BUFSIZE);

		OPENSSL_assert(strlen(objstr)+23+2*enc->iv_len+13 <= sizeof buf);

		buf[0]='\0';
		PEM_proc_type(buf,PEM_TYPE_ENCRYPTED);
		PEM_dek_info(buf,objstr,enc->iv_len,(char *)iv);
		/* k=strlen(buf); */

		EVP_CIPHER_CTX_init(&ctx);
		EVP_EncryptInit_ex(&ctx,enc,NULL,key,iv);
		EVP_EncryptUpdate(&ctx,data,&j,data,i);
		EVP_EncryptFinal_ex(&ctx,&(data[j]),&i);
		EVP_CIPHER_CTX_cleanup(&ctx);
		i+=j;
		ret=1;
	}
	else
	{
		ret=1;
		buf[0]='\0';
	}
	i=PEM_write_bio(bp,name,buf,data,i);
	if (i <= 0) ret=0;
err:
	OPENSSL_cleanse(key,sizeof(key));
	OPENSSL_cleanse(iv,sizeof(iv));
	OPENSSL_cleanse((char *)&ctx,sizeof(ctx));
	OPENSSL_cleanse(buf,PEM_BUFSIZE);
	if (data != NULL)
	{
		OPENSSL_cleanse(data,(unsigned int)dsize);
		OPENSSL_free(data);
	}
	return(ret);
}
