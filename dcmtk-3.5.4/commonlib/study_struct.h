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
	char study_date[16], modalities[24], path[96], uid[65], hash[9];

	_tagStudy(const char *puid, const char *studyDate, const char *mods, size_t s);
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
    
    std::string modalities;
    std::string studyDates;

	_tagVolume(const size_t seq, const size_t volumeSize, const char *desc);
	_tagVolume(const struct _tagVolume &vol);
	_tagVolume(const size_t seq, const size_t volumeSize, const char *desc, const Study &study, std::ostream &errlog);

	bool push_back(const Study &study, std::ostream &errlog);
	void print(std::ostream &strm) const;
    void sort_and_modalities_study();
} Volume;
