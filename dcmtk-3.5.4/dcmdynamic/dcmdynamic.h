// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 DCMDYNAMIC_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// DCMDYNAMIC_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifdef DCMDYNAMIC_EXPORTS
#define DCMDYNAMIC_API __declspec(dllexport)
#else
#define DCMDYNAMIC_API __declspec(dllimport)
#endif
extern "C" {
DCMDYNAMIC_API int __stdcall MergeDicomDir(const char *fileNames, const char *opt_output, const char *opt_fileset, const char *opt_class_uid_prefix, std::ostream &errlog, bool verbose);
DCMDYNAMIC_API int __stdcall MergeDicomDirCerr(const char *fileNames, const char *opt_output, const char *opt_fileset, const char *opt_class_uid_prefix, bool verbose);
DCMDYNAMIC_API bool __stdcall DicomDir2Xml(const char *dirfile, const char *xmlfile);
}
