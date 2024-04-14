#include <Windows.h>

#include <stdio.h>

static wchar_t szLibraryPath[512];

extern "C"
NTSTATUS
NTAPI
RtlGetLastNtStatus(
    VOID
);

#ifdef _WIN64
#pragma comment(lib, "..\\dbg\\ntdll\\ntdll_x64.lib")
#else
#pragma comment(lib, "..\\dbg\\ntdll\\ntdll_x86.lib")
#endif // _WIN64

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd
)
{
    wchar_t szTemp[256];
    swprintf_s(szTemp, L"Local\\szLibraryName%X", (unsigned int)GetCurrentProcessId());
    HANDLE hMapFile = OpenFileMappingW(FILE_MAP_READ, false, szTemp);
    if(hMapFile)
    {
        const wchar_t* szLibraryPathMapping = (const wchar_t*)MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, sizeof(szLibraryPath));
        if(szLibraryPathMapping)
        {
            lstrcpyW(szLibraryPath, szLibraryPathMapping);
            UnmapViewOfFile(szLibraryPathMapping);
        }
        CloseHandle(hMapFile);
    }

    if(szLibraryPath[0] == L'\0')
    {
        // NOTE: No MessageBoxW here on purpose (enables DLL sideloading)
        return RtlGetLastNtStatus();
    }

    HINSTANCE hDll = LoadLibraryW(szLibraryPath);
    if(hDll == nullptr)
    {
        auto lastStatus = RtlGetLastNtStatus();
        swprintf_s(szTemp, L"Failed to load DLL", GetLastError());
        MessageBoxW(0, szLibraryPath, szTemp, MB_ICONERROR | MB_SYSTEMMODAL);
        return lastStatus;
    }
    else
    {
        swprintf_s(szTemp, L"DLL loaded: 0x%p", hDll);
        MessageBoxW(0, szLibraryPath, szTemp, MB_ICONINFORMATION | MB_SYSTEMMODAL);
        return EXIT_SUCCESS;
    }
}
