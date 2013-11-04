#pragma once

extern "C"
{
	int fillSeedSIV(void *siv, size_t sivSize, void *content, size_t contentLength, size_t start);
	void MD5_digest(void *data, size_t dataLength, unsigned char *md);
	unsigned int getLockNumber(const char *filter, const char *regxPattern, int isDirectory, char *lockname);
}
