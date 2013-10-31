#include <direct.h>
#include <openssl/err.h>
#include <openssl/ossl_typ.h>
#include <openssl/ui.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/md5.h>

#include "apps.h"
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

static UI_METHOD *ui_method = NULL;

static int ui_open(UI *ui)
{
	return UI_method_get_opener(UI_OpenSSL())(ui);
}

static int ui_read(UI *ui, UI_STRING *uis)
{
	if (UI_get_input_flags(uis) & UI_INPUT_FLAG_DEFAULT_PWD
		&& UI_get0_user_data(ui))
	{
		switch(UI_get_string_type(uis))
		{
		case UIT_PROMPT:
		case UIT_VERIFY:
			{
				const char *password = (const char*)
					((PW_CB_DATA *)UI_get0_user_data(ui))->password;
				if (password && password[0] != '\0')
				{
					UI_set_result(ui, uis, password);
					return 1;
				}
			}
		default:
			break;
		}
	}
	return UI_method_get_reader(UI_OpenSSL())(ui, uis);
}

static int ui_write(UI *ui, UI_STRING *uis)
{
	if (UI_get_input_flags(uis) & UI_INPUT_FLAG_DEFAULT_PWD
		&& UI_get0_user_data(ui))
	{
		switch(UI_get_string_type(uis))
		{
		case UIT_PROMPT:
		case UIT_VERIFY:
			{
				const char *password = (const char*)
					((PW_CB_DATA *)UI_get0_user_data(ui))->password;
				if (password && password[0] != '\0')
					return 1;
			}
		default:
			break;
		}
	}
	return UI_method_get_writer(UI_OpenSSL())(ui, uis);
}

static int ui_close(UI *ui)
{
	return UI_method_get_closer(UI_OpenSSL())(ui);
}

int setup_ui_method(void)
{
	ui_method = UI_create_method("OpenSSL application user interface");
	UI_method_set_opener(ui_method, ui_open);
	UI_method_set_reader(ui_method, ui_read);
	UI_method_set_writer(ui_method, ui_write);
	UI_method_set_closer(ui_method, ui_close);
	return 0;
}

static void destroy_ui_method(void)
	{
	if(ui_method)
		{
		UI_destroy_method(ui_method);
		ui_method = NULL;
		}
	}
static int password_callback(char *buf, int bufsiz, int verify,
	PW_CB_DATA *cb_tmp)
	{
	UI *ui = NULL;
	int res = 0;
	const char *prompt_info = NULL;
	const char *password = NULL;
	PW_CB_DATA *cb_data = (PW_CB_DATA *)cb_tmp;

	if (cb_data)
	{
		if (cb_data->password)
			password = (const char*)cb_data->password;
		if (cb_data->prompt_info)
			prompt_info = cb_data->prompt_info;
	}

	if (password)
	{
		res = strlen(password);
		if (res > bufsiz)
			res = bufsiz;
		memcpy(buf, password, res);
		return res;
	}

	ui = UI_new_method(ui_method);
	if (ui)
	{
		int ok = 0;
		char *buff = NULL;
		int ui_flags = 0;
		char *prompt = NULL;

		prompt = UI_construct_prompt(ui, "pass phrase",
			prompt_info);

		ui_flags |= UI_INPUT_FLAG_DEFAULT_PWD;
		UI_ctrl(ui, UI_CTRL_PRINT_ERRORS, 1, 0, 0);

		if (ok >= 0)
			ok = UI_add_input_string(ui,prompt,ui_flags,buf,
				PW_MIN_LENGTH,BUFSIZ-1);
		if (ok >= 0 && verify)
			{
			buff = (char *)OPENSSL_malloc(bufsiz);
			ok = UI_add_verify_string(ui,prompt,ui_flags,buff,
				PW_MIN_LENGTH,BUFSIZ-1, buf);
			}
		if (ok >= 0)
			do
			{
				ok = UI_process(ui);
			}
			while (ok < 0 && UI_ctrl(ui, UI_CTRL_IS_REDOABLE, 0, 0, 0));

		if (buff)
		{
			OPENSSL_cleanse(buff,(unsigned int)bufsiz);
			OPENSSL_free(buff);
		}

		if (ok >= 0)
			res = strlen(buf);
		if (ok == -1)
		{
			BIO_printf(bio_err, "User interface error\n");
			ERR_print_errors(bio_err);
			OPENSSL_cleanse(buf,(unsigned int)bufsiz);
			res = 0;
		}
		if (ok == -2)
		{
			BIO_printf(bio_err,"aborted!\n");
			OPENSSL_cleanse(buf,(unsigned int)bufsiz);
			res = 0;
		}
		UI_free(ui);
		OPENSSL_free(prompt);
	}
	return res;
}

static int load_config(BIO *err, CONF *cnf)
{
	if (!cnf)
		cnf = config;
	if (!cnf)
		return 1;

	OPENSSL_load_builtin_modules();

	if (CONF_modules_load(cnf, NULL, 0) <= 0)
		{
		BIO_printf(err, "Error configuring OpenSSL\n");
		ERR_print_errors(err);
		return 0;
		}
	return 1;
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

static EVP_PKEY *load_pubkey(BIO *err, const char *file, int format, int maybe_stdin,
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
			pkey = ENGINE_load_public_key(e, file,
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
		BIO_printf(err, "Error opening %s %s\n",
			key_descrip, file);
		ERR_print_errors(err);
		goto end;
	}

	if (format == FORMAT_ASN1)
	{
		pkey=d2i_PUBKEY_bio(key, NULL);
	}
	else if (format == FORMAT_PEM)
	{
		pkey=PEM_read_bio_PUBKEY(key,NULL,
			(pem_password_cb *)password_callback, &cb_data);
	}
#if defined(PACS_USING_FORMAT_NETSCAPE_OR_PKCS12)
#if !defined(OPENSSL_NO_RC4) && !defined(OPENSSL_NO_RSA)
	else if (format == FORMAT_NETSCAPE || format == FORMAT_IISSGC)
		pkey = load_netscape_key(err, key, file, key_descrip, format);
#endif
#endif
	else
	{
		BIO_printf(err,"bad input format specified for key file\n");
		goto end;
	}
end:
	if (key != NULL) BIO_free(key);
	if (pkey == NULL)
		BIO_printf(err,"unable to load %s\n", key_descrip);
	return(pkey);
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

static RSA *loadPublicKey(char *publicKey)
{
	RSA *rsa = NULL;
	EVP_PKEY *pkey = load_pubkey(bio_err, publicKey, FORMAT_PEM, 0, NULL, NULL, "Public Key");
	if(pkey)
	{
		rsa = EVP_PKEY_get1_RSA(pkey);
		EVP_PKEY_free(pkey);
	}
	return rsa;
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

	apps_startup();
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

int rsaSignVerify(char *infile, char *outfile, char *keyfile, int keytype, char *pass)
{
	RSA *rsa = NULL;
	BIO *in = NULL, *out = NULL;
	unsigned char *rsa_in = NULL, *rsa_out = NULL, rev = 0;
	int rsa_inlen, rsa_outlen = 0, keysize;
	int ret = 1;

	apps_startup();
	app_RAND_load_file(NULL, bio_err, 0);

	if(keytype == KEY_PRIVKEY)
		rsa = loadCheckPrivateKey(keyfile, TRUE, pass);
	else
		rsa = loadPublicKey(keyfile);
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
	
	if(keytype == KEY_PRIVKEY)
		rsa_outlen = RSA_private_encrypt(rsa_inlen, rsa_in, rsa_out, rsa, RSA_PKCS1_PADDING);
	else
		rsa_outlen  = RSA_public_decrypt(rsa_inlen, rsa_in, rsa_out, rsa, RSA_PKCS1_PADDING);
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

#define  BUFFER_SIZE 256
static const char magic[] = "Salted__";
int aes256cbc_enc(char *outf, unsigned char *pass, size_t passLength)
{
	const EVP_CIPHER *cipher=NULL;
	unsigned char salt[PKCS5_SALT_LEN], key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH]; // { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46 };
	BIO *out=NULL,*b64=NULL,*benc=NULL,*wbio=NULL;
	EVP_CIPHER_CTX *ctx = NULL;
	const EVP_MD *dgst=NULL;
	int ret = 1;
	unsigned char buf[BUFFER_SIZE];
	int i = 0;

	for(i = 0; i < BUFFER_SIZE; ++i) buf[i] = i;
	apps_startup();
	if (!load_config(bio_err, NULL))
		goto aes_end;
	cipher=EVP_get_cipherbyname("aes-256-cbc");
	dgst = EVP_md5();
	out=BIO_new(BIO_s_file());
	if (BIO_write_filename(out,outf) <= 0)
	{
		perror(outf);
		goto aes_end;
	}
	wbio = out;
	if ((b64=BIO_new(BIO_f_base64())) == NULL)
		goto aes_end;
	wbio=BIO_push(b64,wbio);

	RAND_pseudo_bytes(salt, PKCS5_SALT_LEN);
	BIO_write(wbio, magic, sizeof(magic) -1 );
	BIO_write(wbio, (char *)salt, PKCS5_SALT_LEN);
	EVP_BytesToKey(cipher, dgst, salt, pass, passLength, ENCRYPT, key, iv);

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

	wbio=BIO_push(benc, wbio);
	ret = BIO_write(wbio, buf, BUFFER_SIZE);
	BIO_flush(wbio);
	BIO_printf(bio_err,"bytes written:%8ld\n",BIO_number_written(out));
	ret = 0;
aes_end:
	if (benc != NULL) BIO_free(benc);
	if (b64 != NULL) BIO_free(b64);
	if (out != NULL) BIO_free_all(out);
	apps_shutdown();
	return ret;
}

int aes256cbc_dec(char *inf, unsigned char *pass, size_t passLength)
{
	const EVP_CIPHER *cipher=NULL;
	unsigned char salt[PKCS5_SALT_LEN], key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH]; // { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46 };
	unsigned char buf[BUFFER_SIZE], mbuf[sizeof(magic) - 1], *linebuffer = NULL, *data = NULL;
	BIO *in=NULL,*b64=NULL,*benc=NULL,*rbio=NULL, *out=NULL, *wbio=NULL;
	EVP_CIPHER_CTX *ctx = NULL;
	const EVP_MD *dgst=NULL;
	BUF_MEM *memptr = NULL;
	int ret = 1, i = 0;
	long dataLength = 0;

	for(i = 0; i < BUFFER_SIZE; ++i) buf[i] = i;
	apps_startup();
	if (!load_config(bio_err, NULL))
		goto aes_dec_end;
	cipher=EVP_get_cipherbyname("aes-256-cbc");
	dgst = EVP_md5();

	//out = BIO_new_mem_buf(buf, BUFFER_SIZE);
	out = BIO_new(BIO_s_mem());
	wbio = out;
	in = BIO_new(BIO_s_file());
	ret = BIO_read_filename(in, inf);
	rbio = in;

	if ((b64=BIO_new(BIO_f_base64())) == NULL)
		goto aes_dec_end;
	rbio = BIO_push(b64, rbio);

	if(BIO_read(rbio,mbuf,sizeof(mbuf)) != sizeof(mbuf)
		|| BIO_read(rbio, (unsigned char *)salt, sizeof(salt)) != sizeof(salt))
		goto aes_dec_end;
	EVP_BytesToKey(cipher, dgst, salt, pass, passLength, DECRYPT, key, iv);

	if ((benc=BIO_new(BIO_f_cipher())) == NULL)
		goto aes_dec_end;
	BIO_get_cipher_ctx(benc, &ctx);
	if (!EVP_CipherInit_ex(ctx, cipher, NULL, NULL, NULL, DECRYPT))
	{
		BIO_printf(bio_err, "Error setting cipher %s\n", EVP_CIPHER_name(cipher));
		ERR_print_errors(bio_err);
		goto aes_dec_end;
	}
	//if (no_padding)
	//		EVP_CIPHER_CTX_set_padding(ctx, 0);
	if (!EVP_CipherInit_ex(ctx, NULL, NULL, key, iv, DECRYPT))
	{
		BIO_printf(bio_err, "Error setting cipher %s\n", EVP_CIPHER_name(cipher));
		ERR_print_errors(bio_err);
		goto aes_dec_end;
	}
	wbio = BIO_push(benc, wbio);

	linebuffer = (unsigned char *)OPENSSL_malloc(EVP_ENCODE_LENGTH(BUFFER_SIZE));
	do
	{
		i = BIO_read(rbio, linebuffer, EVP_ENCODE_LENGTH(BUFFER_SIZE));
		if(i > 0)
			BIO_write(wbio, linebuffer, i);
	} while(i > 0);

	dataLength = BIO_get_mem_data(out, &data);
	for(i = 0; i < dataLength; ++i)
	{
		if(i != data[i])
		{
			ret = 1;
			goto aes_dec_end;
		}
	}
	ret = 0;
aes_dec_end:
	if(linebuffer != NULL) OPENSSL_free(linebuffer);
	if (benc != NULL) BIO_free(benc);
	if (b64 != NULL) BIO_free(b64);
	if (in != NULL) BIO_free(in);
	if (out != NULL) BIO_free(out);
	apps_shutdown();
	return ret;
}

void base64test()
{
	const char *filename = "base64.txt";
	BIO *mbio,*b64bio,*bio = NULL;
	unsigned char buf[BUFFER_SIZE];
	int i = 0;
	for(i = 0; i < BUFFER_SIZE; ++i)
		buf[i] = i;
	
	mbio = BIO_new(BIO_s_file());
	if (BIO_write_filename(mbio, (void*)filename) <= 0)
	{
		perror(filename);
		goto err_base64;
	}
	b64bio=BIO_new(BIO_f_base64());
	bio=BIO_push(b64bio,mbio);
	BIO_write(bio, buf, sizeof(buf));
	BIO_flush(bio);
	BIO_free_all(bio);
	bio = NULL;

	memset(buf, 0, BUFFER_SIZE);
	mbio = BIO_new(BIO_s_file());
	if (BIO_read_filename(mbio, (void*)filename) <= 0)
	{
		perror(filename);
		goto err_base64;
	}
	b64bio=BIO_new(BIO_f_base64());
	bio=BIO_push(b64bio,mbio);
	BIO_read(bio, buf, BUFFER_SIZE);
err_base64:
	if(bio) BIO_free_all(bio);
}

int fillSeedSIV(void *siv, size_t sivSize, void *content, size_t contentLength, size_t start)
{
	int read = 0;
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
