#include <direct.h>
#include <openssl/err.h>
#include <openssl/ui.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/md5.h>
#include <apps.h>

#include "constant.h"

int PACS_PEM_ASN1_write_bio(i2d_of_void *i2d, const char *name, BIO *bp,
		       char *x, const EVP_CIPHER *enc, unsigned char *kstr,
		       int klen, pem_password_cb *callback, void *u);

static DWORD fileNotExist(const char *filename)
{
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	if(filename == NULL)
		return ERROR_FILE_NOT_FOUND;
	hFind = FindFirstFile(filename, &ffd);
	if(hFind == INVALID_HANDLE_VALUE)
		return GetLastError();
	else
	{
		FindClose(hFind);
		return 0;
	}
}

static int MS_CALLBACK genrsa_cb(int p, int n, BN_GENCB *cb)
{
	char c='*';

	if (p == 0) c='.';
	if (p == 1) c='+';
	if (p == 2) c='*';
	if (p == 3) c='\n';
	BIO_write((BIO*)cb->arg,&c,1);
	(void)BIO_flush((BIO*)cb->arg);
#ifdef LINT
	p=n;
#endif
	return 1;
}

static EVP_PKEY *load_key(BIO *err, const char *file, int format, int maybe_stdin,
	const char *pass, ENGINE *e, const char *key_descrip)
{
	BIO *key=NULL;
	EVP_PKEY *pkey=NULL;
	PW_CB_DATA cb_data;

	cb_data.password = pass;
	cb_data.prompt_info = file;

	if (file == NULL && (!maybe_stdin || format == FORMAT_ENGINE))
	{
		BIO_printf(err,"no keyfile specified\n");
		goto end;
	}
#ifndef OPENSSL_NO_ENGINE
	if (format == FORMAT_ENGINE)
		{
		if (!e)
			BIO_printf(bio_err,"no engine specified\n");
		else
			pkey = ENGINE_load_private_key(e, file,
				ui_method, &cb_data);
		goto end;
		}
#endif
	key=BIO_new(BIO_s_file());
	if (key == NULL)
	{
		ERR_print_errors(err);
		goto end;
	}
	if (file == NULL && maybe_stdin)
	{
		setvbuf(stdin, NULL, _IONBF, 0);
		BIO_set_fp(key,stdin,BIO_NOCLOSE);
	}
	else if (BIO_read_filename(key,file) <= 0)
	{
		BIO_printf(err, "Error opening %s %s\n", key_descrip, file);
		ERR_print_errors(err);
		goto end;
	}
	if (format == FORMAT_ASN1)
	{
		pkey=d2i_PrivateKey_bio(key, NULL);
	}
	else if (format == FORMAT_PEM)
	{
		pkey=PEM_read_bio_PrivateKey(key,NULL, (pem_password_cb *)password_callback, &cb_data);
	}
#if defined(PACS_USING_FORMAT_NETSCAPE_OR_PKCS12)
#if !defined(OPENSSL_NO_RC4) && !defined(OPENSSL_NO_RSA)
	else if (format == FORMAT_NETSCAPE || format == FORMAT_IISSGC)
		pkey = load_netscape_key(err, key, file, key_descrip, format);
#endif
	else if (format == FORMAT_PKCS12)
	{
		if (!load_pkcs12(err, key, key_descrip,
				(pem_password_cb *)password_callback, &cb_data,
				&pkey, NULL, NULL))
			goto end;
	}
	else
	{
		BIO_printf(err,"bad input format specified for key file\n");
		goto end;
	}
#endif
 end:
	if (key != NULL) BIO_free(key);
	if (pkey == NULL)
		BIO_printf(err,"unable to load %s\n", key_descrip);
	return(pkey);
}

static RSA *loadCheckPrivateKey(char *privateKey, BOOLEAN check, char *pass)
{
	RSA *rsaLoadPrivate = NULL;
	{
		EVP_PKEY *pkey = load_key(bio_err, privateKey, FORMAT_PEM, 1, pass, NULL, "Private Key");
		if (pkey != NULL)
			rsaLoadPrivate = pkey == NULL ? NULL : EVP_PKEY_get1_RSA(pkey);
		EVP_PKEY_free(pkey);
	}

	if (rsaLoadPrivate == NULL)
	{
		ERR_print_errors(bio_err);
		return NULL;
	}
	else if(check)
	{
		//check loaded private key
		int r = RSA_check_key(rsaLoadPrivate);
		if (r == 1)
			BIO_printf(bio_err, "RSA private key check ok\n");
		else if (r == 0)
		{
			unsigned long err;
			while ((err = ERR_peek_error()) != 0 &&
				ERR_GET_LIB(err) == ERR_LIB_RSA &&
				ERR_GET_FUNC(err) == RSA_F_RSA_CHECK_KEY &&
				ERR_GET_REASON(err) != ERR_R_MALLOC_FAILURE)
			{
				BIO_printf(bio_err, "RSA private key error: %s\n", ERR_reason_error_string(err));
				ERR_get_error(); // remove e from error stack
			}
		}

		if (r == -1 || ERR_peek_error() != 0) // should happen only if r == -1
		{
			ERR_print_errors(bio_err);
			return NULL;
		}
	}
	return rsaLoadPrivate;
}

int genrsa(int num, char *privateKey, char *publicKey, char *passout)
{
	unsigned long f4 = RSA_F4;
	long l = 0L;
	int i, ret = 1;
	BN_GENCB cb;
	BIGNUM *bn = BN_new();
	const EVP_CIPHER *enc = EVP_aes_256_cbc();
	RSA *rsa = NULL, *rsaPublic = NULL;
	BIO *out = NULL;

	BIO_printf(bio_err,"app starting...\n");
	apps_startup();
	BIO_printf(bio_err,"app start\n");
	BN_GENCB_set(&cb, genrsa_cb, bio_err);
	if (bio_err == NULL)
		if ((bio_err=BIO_new(BIO_s_file())) != NULL)
			BIO_set_fp(bio_err, stderr, BIO_NOCLOSE|BIO_FP_TEXT);

	if (!load_config(bio_err, NULL))
		goto err;
	if ((out = BIO_new(BIO_s_file())) == NULL)
	{
		BIO_printf(bio_err,"unable to create BIO for output\n");
		goto err;
	}

	if (privateKey == NULL)
	{
		BIO_set_fp(out, stdout, BIO_NOCLOSE);
	}
	else
	{
		if(fileNotExist(privateKey))
		{
			if(BIO_write_filename(out, privateKey) <= 0)
			{
				perror(privateKey);
				goto err;
			}
		}
		else
		{
			BIO_printf(bio_err,"using existing private key\n");
			rsa = loadCheckPrivateKey(privateKey, TRUE, passout);
			if(rsa == NULL && BIO_write_filename(out, privateKey) <= 0)
			{
				perror(privateKey);
				goto err;
			}
		}
	}

	if(rsa == NULL)
	{	// create new private key
		if (!app_RAND_load_file(NULL, bio_err, 1) && !RAND_status())
			BIO_printf(bio_err,"warning, not much extra random data, consider using the -rand option\n");
		BIO_printf(bio_err,"Generating RSA private key, %d bit long modulus\n",	num);

		BN_set_word(bn, f4);
		rsa = RSA_new();
		RSA_generate_key_ex(rsa, num, bn, &cb);
		app_RAND_write_file(NULL, bio_err);

		for (i = 0; i < rsa->e->top; ++i)
		{
#ifndef SIXTY_FOUR_BIT
			l <<= BN_BITS4;
			l <<= BN_BITS4;
#endif
			l += rsa->e->d[i];
		}
		BIO_printf(bio_err,"e is %ld (0x%lX)\nwriting RSA private key\n",l,l);
		{
			PW_CB_DATA cb_data;
			cb_data.password = passout;
			cb_data.prompt_info = privateKey;
			if(!PACS_PEM_ASN1_write_bio((i2d_of_void*)i2d_RSAPrivateKey,PEM_STRING_RSA,
				out, (char*)rsa, enc, NULL, 0, (pem_password_cb *)password_callback, &cb_data))
				goto err;
			//if (!PEM_write_bio_RSAPrivateKey(out, rsa, enc, NULL, 0, (pem_password_cb *)password_callback, &cb_data))
			//	goto err;
		}
		BIO_printf(bio_err,"create new RSA private key: %s\n", privateKey);
	}
	else
		BIO_printf(bio_err,"use existing RSA private key: %s\n", privateKey);

	if(out != NULL)
	{
		BIO_free_all(out); // flush out, complete writing private key
		out = NULL;
	}

	// private key OK, export public key, set output to public key file
	out = BIO_new(BIO_s_file());
	if (BIO_write_filename(out, publicKey) <= 0)
	{
		perror(publicKey);
		goto err;
	}
	BIO_printf(bio_err,"writing RSA public key\n");
	i = PEM_write_bio_RSA_PUBKEY(out, rsa);
	if (!i)
	{
		BIO_printf(bio_err,"unable to write key\n");
		ERR_print_errors(bio_err);
		goto err;
	}
	BIO_printf(bio_err,"RSA public key OK\n");

	ret=0;
err:
	if (bn) BN_free(bn);
	if (rsa) RSA_free(rsa);
	if (rsaPublic) RSA_free(rsaPublic);
	if (out) BIO_free_all(out);
	//if(passout) OPENSSL_free(passout);
	if (ret != 0)
		ERR_print_errors(bio_err);
	apps_shutdown();
	return ret;
}

int rsaSign(char *infile, char *outfile, char *keyfile, char *pass)
{
	RSA *rsa = NULL;
	BIO *in = NULL, *out = NULL;
	unsigned char *rsa_in = NULL, *rsa_out = NULL, rev = 0;
	int rsa_inlen, rsa_outlen = 0, keysize;
	int ret = 1;

	apps_startup();
	app_RAND_load_file(NULL, bio_err, 0);

	rsa = loadCheckPrivateKey(keyfile, TRUE, pass);

	if(!rsa)
	{
		BIO_printf(bio_err, "Error getting RSA key\n");
		ERR_print_errors(bio_err);
		goto sign_err;
	}

	if(infile)
	{
		if(!(in = BIO_new_file(infile, "rb")))
		{
			BIO_printf(bio_err, "Error Reading Input File\n");
			ERR_print_errors(bio_err);	
			goto sign_err;
		}
	}
	else
		in = BIO_new_fp(stdin, BIO_NOCLOSE);

	if(outfile)
	{
		if(!(out = BIO_new_file(outfile, "wb")))
		{
			BIO_printf(bio_err, "Error Reading Output File\n");
			ERR_print_errors(bio_err);	
			goto sign_err;
		}
	}
	else
		out = BIO_new_fp(stdout, BIO_NOCLOSE);

	keysize = RSA_size(rsa);
	rsa_in = (unsigned char*)OPENSSL_malloc(keysize * 2);
	rsa_out = (unsigned char*)OPENSSL_malloc(keysize);

	/* Read the input data */
	rsa_inlen = BIO_read(in, rsa_in, keysize * 2);
	if(rsa_inlen <= 0)
	{
		BIO_printf(bio_err, "Error reading input Data\n");
		goto sign_err;
	}
	
	rsa_outlen = RSA_private_encrypt(rsa_inlen, rsa_in, rsa_out, rsa, RSA_PKCS1_PADDING);

	if(rsa_outlen <= 0)
	{
		BIO_printf(bio_err, "RSA operation error\n");
		ERR_print_errors(bio_err);
		goto sign_err;
	}
	BIO_write(out, rsa_out, rsa_outlen);
	ret = 0;
sign_err:
	if (rsa) RSA_free(rsa);
	if (in) BIO_free_all(in);
	if (out) BIO_free_all(out);
	if(rsa_in) OPENSSL_free(rsa_in);
	if(rsa_out) OPENSSL_free(rsa_out);
	if (ret != 0)
		ERR_print_errors(bio_err);
	apps_shutdown();
	return ret;
}

static const char magic[] = "Salted__";
int aes256cbc_enc(void *content, size_t contentLength, char *filename, unsigned char *key, unsigned char* iv)
{
	const EVP_CIPHER *cipher=NULL;
	unsigned char salt[PKCS5_SALT_LEN], *writtenBuffer = NULL;  //, key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH];
	BIO *out = NULL, *benc = NULL, *wbio = NULL;
	EVP_CIPHER_CTX *ctx = NULL;
	const EVP_MD *dgst=NULL;
	int ret = 0;

	apps_startup();
	if (!load_config(bio_err, NULL))
		goto aes_end;
	cipher=EVP_get_cipherbyname("aes-256-cbc");
	dgst = EVP_md5();
	out = BIO_new(BIO_s_file());
	BIO_write_filename(out, filename);
	wbio = out;

	RAND_pseudo_bytes(salt, PKCS5_SALT_LEN);
	BIO_write(wbio, magic, sizeof(magic) -1 );
	BIO_write(wbio, (char *)salt, PKCS5_SALT_LEN);
	//EVP_BytesToKey(cipher, dgst, salt, pass, passLength, ENCRYPT, key, iv);

	if ((benc=BIO_new(BIO_f_cipher())) == NULL)
		goto aes_end;
	BIO_get_cipher_ctx(benc, &ctx);
	if (!EVP_CipherInit_ex(ctx, cipher, NULL, NULL, NULL, ENCRYPT))
	{
		BIO_printf(bio_err, "Error setting cipher %s\n", EVP_CIPHER_name(cipher));
		ERR_print_errors(bio_err);
		goto aes_end;
	}
	//if (no_padding)
	//		EVP_CIPHER_CTX_set_padding(ctx, 0);
	if (!EVP_CipherInit_ex(ctx, NULL, NULL, key, iv, ENCRYPT))
	{
		BIO_printf(bio_err, "Error setting cipher %s\n", EVP_CIPHER_name(cipher));
		ERR_print_errors(bio_err);
		goto aes_end;
	}
	wbio = BIO_push(benc, wbio);
	ret = BIO_write(wbio, content, contentLength);
	//BIO_printf(bio_err,"bytes written:%8ld\n", ret);
	BIO_flush(wbio);
aes_end:
	if (benc != NULL) BIO_free(benc);
	if (out != NULL) BIO_free_all(out);
	apps_shutdown();
	return ret;
}
