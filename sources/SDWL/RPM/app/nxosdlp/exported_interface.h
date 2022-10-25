#pragma once

#ifdef NXOSDLP_EXPORTS
#define NXOSDLP_API  extern "C" _declspec(dllexport)
#else
#define NXOSDLP_API _declspec(dllimport)
#endif // NXOSDLP_EXPORTS



NXOSDLP_API bool __stdcall init_file_virtualization(HANDLE *outhandler);

NXOSDLP_API bool __stdcall set_fv_path(HANDLE handler,const wchar_t* target, const wchar_t* redirected);
