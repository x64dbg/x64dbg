#include <windows.h>

wchar_t szLibraryPath[512];

int main()
{
    wchar_t szName[256];
    wsprintfW(szName, L"Local\\szLibraryName%X", (unsigned int)GetCurrentProcessId());
    HANDLE hMapFile = OpenFileMappingW(FILE_MAP_READ, false, szName);
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
    if(szLibraryPath[0])
        return (LoadLibraryW(szLibraryPath) != NULL);
    return 0;
}
