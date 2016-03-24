#include "stdafx.h"
#include "commonlib.h"
#include "study_struct.h"
using namespace std;

_tagStudy::_tagStudy(const char *puid, size_t s) : size(s)
{
	if(puid)
		strcpy_s(uid, sizeof(uid), puid);
	else
		throw exception("检查UID不能为空");
	__int64 hashStudy = HashStr(puid, hash, sizeof(hash));
	sprintf_s(path, sizeof(path), "archdir\\%c%c\\%c%c\\%c%c\\%c%c\\%s",
		hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7], uid);
	string dicomdirPath;
	dicomdirPath.reserve(128);
	dicomdirPath = path;
	dicomdirPath.append("\\DICOMDIR");
	if(_access_s(dicomdirPath.c_str(), 4))
	{
		dicomdirPath.append(" 不存在");
		throw exception(dicomdirPath.c_str());
	}
	dicomdirPath.erase(strlen(path), string::npos);
	dicomdirPath.append(1, '\\').append(hash);
	if(_access_s(dicomdirPath.c_str(), 0))
	{
		dicomdirPath.append(" 不存在");
		throw exception(dicomdirPath.c_str());
	}
	if(size == 0)
		size = diskUsage("..", uid) / (1024 * 1024) + 1;
}

_tagStudy::_tagStudy(const Study &s)
{
	memcpy_s(this, sizeof(_tagStudy), &s, sizeof(_tagStudy));
}

_tagVolume::_tagVolume(const size_t seq, const size_t volumeSize, const char *desc)
	: sequence(seq), total(volumeSize), remain(volumeSize), description(desc), valid(false) {}

_tagVolume::_tagVolume(const Volume &vol)
	: sequence(vol.sequence), total(vol.total), remain(vol.remain), volumeCount(vol.volumeCount),
	description(vol.description), valid(vol.valid), studiesOnVolume(vol.studiesOnVolume) {}

_tagVolume::_tagVolume(const size_t seq, const size_t volumeSize, const char *desc, const Study &study, ostream &errlog)
	: sequence(seq), total(volumeSize), description(desc), valid(false)
{
	studiesOnVolume.push_back(study);
	if(total < study.size)
	{
		remain = 0;
		errlog << study.size << "MB 超出分卷大小(" << total << "MB)" << endl;
	}
	else
		remain = total - study.size;
}

bool _tagVolume::push_back(const Study &study, ostream &errlog)
{
	if(remain < study.size)
	{
		errlog << "分卷空间(" << remain << "MB)不足, 无法容纳检查(" << study.size << "MB)" << study.uid << std::endl;
		return false;
	}
	remain -= study.size;
	studiesOnVolume.push_back(study);
	return true;
}

void _tagVolume::print(std::ostream &strm) const
{
	strm << description << " " << sequence << "/" << volumeCount << ", 分卷大小: " << total 
		<< "MB, 剩余: " << remain << "MB, 共" << studiesOnVolume.size() << "个检查:" << endl;
	for_each(studiesOnVolume.begin(), studiesOnVolume.end(), [&strm](const Study &s){
		strm << "\t" << s.size << "MB\t" << s.uid << endl;
	});
	strm << endl;
}
