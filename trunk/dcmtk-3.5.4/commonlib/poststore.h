#pragma once

#define DCM_StudyDate	"0008,0020"
#define DCM_PatientID	"0010,0020"
#define MK_TAG_STRING(key) key

int hashCode(const char *s);
int hashCodeW(const wchar_t *s);
