#include <openssl/err.h>
#include <openssl/ui.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/md5.h>
#include "apps.h"
#include "liblock.h"
#include "openssl_type.h"

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

void destroy_ui_method(void)
{
	if(ui_method)
		{
		UI_destroy_method(ui_method);
		ui_method = NULL;
		}
}

int password_callback(char *buf, int bufsiz, int verify, PW_CB_DATA *cb_tmp)
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

int load_config(BIO *err, CONF *cnf)
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

static RSA *loadPublicKey(const char *publicKey)
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

int rsaVerify(const unsigned char *inBuf, size_t inLen, unsigned char *outBuf, const char *keyfile)
{
	RSA *rsa = NULL;
	int keysize, ret = 0;

	apps_startup();
	app_RAND_load_file(NULL, bio_err, 0);

	rsa = loadPublicKey(keyfile);
	if(!rsa)
	{
		BIO_printf(bio_err, "Error getting RSA key\n");
		ERR_print_errors(bio_err);
		goto sign_err;
	}
	keysize = RSA_size(rsa);
	ret = RSA_public_decrypt(inLen, inBuf, outBuf, rsa, RSA_PKCS1_PADDING);
	if(ret <= 0)
	{
		BIO_printf(bio_err, "RSA operation error\n");
		ERR_print_errors(bio_err);
		goto sign_err;
	}
sign_err:
	if (rsa) RSA_free(rsa);
	apps_shutdown();
	return ret;
}

int aes256cbc_dec(const unsigned char *inBuf, size_t inLen, unsigned char *outBuf, unsigned char *key, unsigned char* iv)
{
	const EVP_CIPHER *cipher=NULL;
	//unsigned char salt[PKCS5_SALT_LEN], key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH];
	unsigned char buf[KEY_SIZE / 8], *data = NULL;
	BIO *benc=NULL, *out=NULL, *wbio=NULL;
	EVP_CIPHER_CTX *ctx = NULL;
	BUF_MEM *memptr = NULL;
	int ret = 1, i = 0;
	long dataLength = 0;

	for(i = 0; i < sizeof(buf); ++i) buf[i] = i;
	apps_startup();
	if (!load_config(bio_err, NULL))
		goto aes_dec_end;
	cipher=EVP_get_cipherbyname("aes-256-cbc");

	//out = BIO_new_mem_buf(buf, BUFFER_SIZE);
	out = BIO_new(BIO_s_mem());
	wbio = out;

	//EVP_BytesToKey(cipher, dgst, salt, pass, passLength, DECRYPT, key, iv);

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
	BIO_write(wbio, inBuf, inLen);
	ret = BIO_get_mem_data(out, &data);
	if(ret <= 0)
	{
		BIO_printf(bio_err, "Error decrypt cipher %s: %d\n", EVP_CIPHER_name(cipher), ret);
		ERR_print_errors(bio_err);
		goto aes_dec_end;
	}
	for(i = 0; i < ret; ++i) outBuf[i] = data[i];
aes_dec_end:
	if (benc != NULL) BIO_free(benc);
	if (out != NULL) BIO_free(out);
	apps_shutdown();
	return ret;
}
