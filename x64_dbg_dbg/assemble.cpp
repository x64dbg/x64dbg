#include "assemble.h"

char nasmpath[deflen]="";

static bool issegment(const char* str)
{
    if(!_strnicmp(str, "gs", 2))
        return true;
    if(!_strnicmp(str, "fs", 2))
        return true;
    if(!_strnicmp(str, "es", 2))
        return true;
    if(!_strnicmp(str, "ds", 2))
        return true;
    if(!_strnicmp(str, "cs", 2))
        return true;
    if(!_strnicmp(str, "ss", 2))
        return true;
    return false;
}

void intel2nasm(const char* intel, char* nasm)
{
    char temp[256]="";
    memset(temp, 0, sizeof(temp));
    int len=strlen(intel);
    for(int i=0,j=0; i<len; i++) //fix basic differences and problems
    {
        if(!_strnicmp(intel+i, "ptr", 3)) //remove "ptr"
        {
            i+=2;
            if(intel[i+1]==' ')
                i++;
            continue;
        }
        else if(!_strnicmp(intel+i, "  ", 2)) //remove double spaces
            continue;
        else if(intel[i]=='\t') //tab=space
        {
            j+=sprintf(temp+j, " ");
            continue;
        }
        j+=sprintf(temp+j, "%c", intel[i]);
    }
    len=strlen(temp);
    for(int i=0,j=0; i<len; i++)
    {
        if(temp[i]==' ' and temp[i+1]==',')
            continue;
        else if(temp[i]==',' and temp[i+1]==' ')
        {
            j+=sprintf(nasm+j, ",");
            i++;
            continue;
        }
        else if(issegment(temp+i) and temp[i+2]==' ')
        {
            j+=sprintf(nasm+j, "%c%c", temp[i], temp[i+1]);
            i+=2;
            continue;
        }
        else if(temp[i]==':' and temp[i+1]==' ')
        {
            j+=sprintf(nasm+j, ":");
            i++;
            continue;
        }
        j+=sprintf(nasm+j, "%c", temp[i]);
    }
    len=strlen(nasm);
    for(int i=0,j=0; i<len; i++)
    {
        if(issegment(nasm+i) and nasm[i+2]==':' and nasm[i+3]=='[')
        {
            j+=sprintf(temp+j, "[%c%c:", nasm[i], nasm[i+1]);
            i+=3;
            continue;
        }
        j+=sprintf(temp+j, "%c", nasm[i]);
    }
    strcpy(nasm, temp);
}

bool assemble(const char* instruction, unsigned char** outdata, int* outsize, char* error, bool x64)
{
    if(!instruction or !outsize)
        return false;
    char tmppath[MAX_PATH]="";
    char tmpfile[MAX_PATH]="";
    char outfile[MAX_PATH]="";
    if(!GetTempPathA(MAX_PATH, tmppath) or !GetTempFileNameA(tmppath, "dbg", 0, tmpfile) or !GetTempFileNameA(tmppath, "dbg", 0, outfile))
    {
        DeleteFileA(outfile);
        DeleteFileA(tmpfile);
        return false;
    }
    char* asmfile=(char*)emalloc(strlen(instruction)+9);
    if(x64)
        strcpy(asmfile, "BITS 64d\n");
    else
        strcpy(asmfile, "BITS 32d\n");
    strcat(asmfile, instruction);
    HANDLE hFile=CreateFileA(tmpfile, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if(hFile==INVALID_HANDLE_VALUE)
    {
        if(error)
            strcpy(error, "Failed to create asmfile!");
        efree(asmfile);
        DeleteFileA(outfile);
        DeleteFileA(tmpfile);
        return false;
    }
    DWORD written=0;
    if(!WriteFile(hFile, asmfile, strlen(asmfile), &written, 0))
    {
        if(error)
            strcpy(error, "Failed to write asmfile!");
        efree(asmfile);
        CloseHandle(hFile);
        DeleteFileA(outfile);
        DeleteFileA(tmpfile);
        return false;
    }
    efree(asmfile);
    CloseHandle(hFile);
    char cmdline[MAX_PATH*4]="";
    sprintf(cmdline, "\"%s\" \"%s\" -o \"%s\"", nasmpath, tmpfile, outfile);
    STARTUPINFO si;
    memset(&si, 0, sizeof(si));
    si.cb=sizeof(si);
    PROCESS_INFORMATION pi;
    if(!CreateProcessA(0, cmdline, 0, 0, false, CREATE_NO_WINDOW, 0, 0, &si, &pi))
    {
        if(error)
            strcpy(error, "Failed to start NASM!");
        DeleteFileA(outfile);
        DeleteFileA(tmpfile);
        return false;
    }
    DWORD exitCode=STILL_ACTIVE;
    do
        GetExitCodeProcess(pi.hProcess, &exitCode);
    while(exitCode==STILL_ACTIVE);
    CloseHandle(pi.hThread);
    if(exitCode) //nasm failed
    {
        if(error)
            strcpy(error, "NASM reported a failure!");
        DeleteFileA(outfile);
        DeleteFileA(tmpfile);
        return false;
    }
    hFile=CreateFileA(outfile, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(hFile==INVALID_HANDLE_VALUE)
    {
        if(error)
            strcpy(error, "Failed to open outfile!");
        DeleteFileA(outfile);
        DeleteFileA(tmpfile);
        return false;
    }
    int filesize=GetFileSize(hFile, 0);
    if(!filesize)
    {
        if(error)
            strcpy(error, "No result in outfile!");
        CloseHandle(hFile);
        DeleteFileA(outfile);
        DeleteFileA(tmpfile);
        return false;
    }
    *outsize=filesize;
    unsigned char* out=(unsigned char*)emalloc(filesize);
    DWORD read=0;
    if(!ReadFile(hFile, out, filesize, &read, 0))
    {
        if(error)
            strcpy(error, "Failed to read outfile!");
        efree(out);
        CloseHandle(hFile);
        DeleteFileA(outfile);
        DeleteFileA(tmpfile);
        return false;
    }
    if(outdata)
        *outdata=out;
    else
        efree(out);
    CloseHandle(hFile);
    DeleteFileA(outfile);
    DeleteFileA(tmpfile);
    return true;
}
