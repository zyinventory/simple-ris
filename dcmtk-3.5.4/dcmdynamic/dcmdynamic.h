// ���� ifdef ���Ǵ���ʹ�� DLL �������򵥵�
// ��ı�׼�������� DLL �е������ļ��������������϶���� DCMDYNAMIC_EXPORTS
// ���ű���ġ���ʹ�ô� DLL ��
// �κ�������Ŀ�ϲ�Ӧ����˷��š�������Դ�ļ��а������ļ����κ�������Ŀ���Ὣ
// DCMDYNAMIC_API ������Ϊ�Ǵ� DLL ����ģ����� DLL ���ô˺궨���
// ������Ϊ�Ǳ������ġ�
#ifdef DCMDYNAMIC_EXPORTS
#define DCMDYNAMIC_API __declspec(dllexport)
#else
#define DCMDYNAMIC_API __declspec(dllimport)
#endif
extern "C" {
DCMDYNAMIC_API int __stdcall MergeDicomDir(const char *fileNames, const char *opt_output, const char *opt_fileset, std::ostream &errlog, bool verbose);
DCMDYNAMIC_API int __stdcall MergeDicomDirCerr(const char *fileNames, const char *opt_output, const char *opt_fileset, bool verbose);
DCMDYNAMIC_API bool __stdcall DicomDir2Xml(const char *dirfile, const char *xmlfile);
}
