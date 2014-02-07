// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the CRC32C_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// CRC32C_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef CRC32C_EXPORTS
#define CRC32C_API __declspec(dllexport)
#else
#define CRC32C_API __declspec(dllimport)
#endif

// This class is exported from the crc32c.dll
class CRC32C_API Ccrc32c {
public:
	Ccrc32c(void);
	// TODO: add your methods here.
};

extern CRC32C_API int ncrc32c;

CRC32C_API int fncrc32c(void);
