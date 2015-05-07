#pragma once

#ifdef COMMONLIB_EXPORTS
#define COMMONLIB_API __declspec(dllexport)
#else
#define COMMONLIB_API __declspec(dllimport)
#endif

typedef struct COMMONLIB_API _tagStudy
{
public:
	size_t size;
	char path[96], uid[65], hash[9];

	_tagStudy(const char *puid, size_t s);
	_tagStudy(const struct _tagStudy &s);
} Study;

typedef struct COMMONLIB_API _tagVolume
{
public:
	const size_t sequence, total;
	const std::string description;
	size_t remain, volumeCount;
	bool valid;
	std::list<Study> studiesOnVolume;

	_tagVolume(const size_t seq, const size_t volumeSize, const char *desc);
	_tagVolume(const struct _tagVolume &vol);
	_tagVolume(const size_t seq, const size_t volumeSize, const char *desc, const Study &study, std::ostream &errlog);

	bool push_back(const Study &study, std::ostream &errlog);
	void print(std::ostream &strm) const;
} Volume;
