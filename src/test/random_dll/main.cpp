#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// Function to generate a random string of specified length
void generateRandomString(char* buffer, int length)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    int charsetSize = sizeof(charset) - 1;

    for(int i = 0; i < length; i++)
    {
        int key = rand() % charsetSize;
        buffer[i] = charset[key];
    }
    buffer[length] = '\0';
}

// Function to copy a file to a new location with a different name
BOOL copyDllWithRandomName(const char* sourceDll, const char* destPath, char* finalPath)
{
    char tempPath[MAX_PATH];
    char randomName[16];

    // Get temp directory if destPath is NULL
    if(destPath == NULL)
    {
        if(GetTempPathA(MAX_PATH, tempPath) == 0)
        {
            printf("Failed to get temp path\n");
            return FALSE;
        }
    }
    else
    {
        strcpy_s(tempPath, MAX_PATH, destPath);
    }

    // Generate random name with .xxl extension (as mentioned in the issue)
    generateRandomString(randomName, 8);
    sprintf_s(finalPath, MAX_PATH, "%s%s.xxl", tempPath, randomName);

    // Copy the DLL to the new location
    if(!CopyFileA(sourceDll, finalPath, FALSE))
    {
        printf("Failed to copy DLL to %s (Error: %lu)\n", finalPath, GetLastError());
        return FALSE;
    }

    return TRUE;
}

int main()
{
    printf("Random DLL Loading Test Case\n");
    printf("============================\n\n");

    printf("\nTo test wildcard DLL breakpoints in x64dbg:\n");
    printf("1. Set a breakpoint with: bpdll *.xxl\n");
    printf("2. Run this program in x64dbg\n");
    printf("3. x64dbg should break when each .xxl file is loaded\n\n");

    // Initialize random seed
    srand(1337);

    // Path to the test DLL (should be compiled first)
    char sourceDll[MAX_PATH];
    GetModuleFileNameA(NULL, sourceDll, MAX_PATH);
    auto slashIdx = strrchr(sourceDll, '\\');
    if(slashIdx)
    {
        slashIdx[1] = '\0';
    }
    strcat_s(sourceDll, "random.dll");
    printf("Source DLL: %s\n", sourceDll);

    char dllPath1[MAX_PATH];
    char dllPath2[MAX_PATH];

    // Try to find the test DLL
    if(GetFileAttributesA(sourceDll) == INVALID_FILE_ATTRIBUTES)
    {
        printf("Error: Could not find %s. Please compile the DLL first.\n", sourceDll);
        return 1;
    }

    // First run - generate and load a random DLL
    printf("=== FIRST RUN ===\n");
    if(!copyDllWithRandomName(sourceDll, NULL, dllPath1))
    {
        printf("Failed to create first random DLL\n");
        return 1;
    }

    printf("Generated random DLL: %s\n", dllPath1);

    HMODULE hDll1 = LoadLibraryA(dllPath1);
    if(hDll1 == NULL)
    {
        printf("Failed to load DLL %s (Error: %lu)\n", dllPath1, GetLastError());
        DeleteFileA(dllPath1);
        return 1;
    }

    printf("Successfully loaded: %s\n", dllPath1);

    // Get and call exported function if available
    typedef void (*TestFunction)(const char*);
    TestFunction testFunc = (TestFunction)GetProcAddress(hDll1, "TestFunction");
    if(testFunc)
    {
        testFunc("Hello from first random DLL!");
    }
    else
    {
        printf("Failed to get TestFunction from DLL %s (Error: %lu)\n", dllPath1, GetLastError());
        return 1;
    }

    // Unload and delete
    FreeLibrary(hDll1);
    DeleteFileA(dllPath1);
    printf("Unloaded and cleaned up first DLL\n\n");

    // Second run - generate and load another random DLL
    printf("=== SECOND RUN ===\n");
    if(!copyDllWithRandomName(sourceDll, NULL, dllPath2))
    {
        printf("Failed to create second random DLL\n");
        return 1;
    }

    printf("Generated random DLL: %s\n", dllPath2);

    HMODULE hDll2 = LoadLibraryA(dllPath2);
    if(hDll2 == NULL)
    {
        printf("Failed to load DLL %s (Error: %lu)\n", dllPath2, GetLastError());
        DeleteFileA(dllPath2);
        return 1;
    }

    printf("Successfully loaded: %s\n", dllPath2);

    // Get and call exported function if available
    testFunc = (TestFunction)GetProcAddress(hDll2, "TestFunction");
    if(testFunc)
    {
        testFunc("Hello from second random DLL!");
    }
    else
    {
        printf("Failed to get TestFunction from DLL %s (Error: %lu)\n", dllPath2, GetLastError());
        return 1;
    }

    // Unload and delete
    FreeLibrary(hDll2);
    DeleteFileA(dllPath2);
    printf("Unloaded and cleaned up second DLL\n\n");

    printf("Test completed!\n");

    return 0;
}