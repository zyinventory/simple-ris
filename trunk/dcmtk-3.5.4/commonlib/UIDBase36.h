#pragma once
#include "stdafx.h"
#include "mini-gmp.h"
using namespace std;

typedef map<string, int> MapString2Int;
typedef map<int, string> MapInt2String;

#define UIDBase36_COMPRESS_LEN 40

class UIDBase36
{
public:
	static const size_t UID_LEN = 64, HEADER_WARNING = 800;

	UIDBase36(void);
	~UIDBase36(void) {
		if(0 == InterlockedDecrement(&UIDBase36::REF_COUNTER))
		{
			MATCH_HEADER_LENGTH.clear();
			uid2index.clear();
			index2uid.clear();
		}
	};
	errno_t compress(const string &uid, char *outputBuffer, size_t bufLen);
	size_t uncompress(const char *b36str, char *outputBuffer);

private:
	static __declspec(align(4)) volatile LONG REF_COUNTER;
	static volatile bool INIT_OK;
	static set<size_t> MATCH_HEADER_LENGTH;
	static MapString2Int uid2index;
	static MapInt2String index2uid;
	
	static void AddStaticDictDirect(const string &key, int code) {
		uid2index.insert(MapString2Int::value_type(key, code));
		index2uid.insert(MapInt2String::value_type(code, key));
		MATCH_HEADER_LENGTH.insert(key.length());
	};

	static errno_t base11_to_base37(const char *src, char *outputBuffer) {
		mpz_t bigint;
		if(mpz_init_set_str(bigint, src, 11)) return EINVAL;
		mpz_get_str(outputBuffer, 37, bigint);
		mpz_clear(bigint);
		return 0;
	};

	static errno_t base37_to_base11(const char *src, char *outputBuffer) {
		mpz_t bigint;
		if(mpz_init_set_str(bigint, src, 37)) return EINVAL;
		mpz_get_str(outputBuffer, -11, bigint); // -11 means: base 11 and upper case
		mpz_clear(bigint);
		return 0;
	};

	static errno_t base11_to_int(const char *src, int *code) {
		mpz_t bigint;
		if(mpz_init_set_str(bigint, src, 11)) return EINVAL;
		*code = mpz_get_si(bigint);
		mpz_clear(bigint);
		return 0;
	};
};
