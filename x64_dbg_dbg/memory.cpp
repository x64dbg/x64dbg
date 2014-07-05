#include "memory.h"
#include "debugger.h"

uint memfindbaseaddr(HANDLE hProcess, uint addr, uint* size)
{
    MEMORY_BASIC_INFORMATION mbi;
    DWORD numBytes;
    uint MyAddress=0, newAddress=0;
    do
    {
        numBytes=VirtualQueryEx(hProcess, (LPCVOID)MyAddress, &mbi, sizeof(mbi));
        newAddress=(uint)mbi.BaseAddress+mbi.RegionSize;
        if(mbi.State==MEM_COMMIT and addr<newAddress and addr>=MyAddress)
        {
            if(size)
                *size=mbi.RegionSize;
            return (uint)mbi.BaseAddress;
        }
        if(newAddress<=MyAddress)
            numBytes=0;
        else
            MyAddress=newAddress;
    }
    while(numBytes);
    return 0;
}

bool memread(HANDLE hProcess, const void* lpBaseAddress, void* lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead)
{
    if(!hProcess or !lpBaseAddress or !lpBuffer or !nSize) //generic failures
        return false;
    SIZE_T read=0;
    DWORD oldprotect=0;
    bool ret=MemoryReadSafe(hProcess, (void*)lpBaseAddress, lpBuffer, nSize, &read); //try 'normal' RPM
    if(ret and read==nSize) //'normal' RPM worked!
    {
        if(lpNumberOfBytesRead)
            *lpNumberOfBytesRead=read;
        return true;
    }
    for(uint i=0; i<nSize; i++) //read byte-per-byte
    {
        unsigned char* curaddr=(unsigned char*)lpBaseAddress+i;
        unsigned char* curbuf=(unsigned char*)lpBuffer+i;
        ret=MemoryReadSafe(hProcess, curaddr, curbuf, 1, 0); //try 'normal' RPM
        if(!ret) //we failed
        {
            if(lpNumberOfBytesRead)
                *lpNumberOfBytesRead=i;
            SetLastError(ERROR_PARTIAL_COPY);
            return false;
        }
    }
    return true;
}

bool memwrite(HANDLE hProcess, void* lpBaseAddress, const void* lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten)
{
    if(!hProcess or !lpBaseAddress or !lpBuffer or !nSize) //generic failures
        return false;
    SIZE_T written=0;
    DWORD oldprotect=0;
    bool ret=MemoryWriteSafe(hProcess, lpBaseAddress, lpBuffer, nSize, &written);
    if(ret and written==nSize) //'normal' WPM worked!
    {
        if(lpNumberOfBytesWritten)
            *lpNumberOfBytesWritten=written;
        return true;
    }
    for(uint i=0; i<nSize; i++) //write byte-per-byte
    {
        unsigned char* curaddr=(unsigned char*)lpBaseAddress+i;
        unsigned char* curbuf=(unsigned char*)lpBuffer+i;
        ret=MemoryWriteSafe(hProcess, curaddr, curbuf, 1, 0); //try 'normal' WPM
        if(!ret) //we failed
        {
            if(lpNumberOfBytesWritten)
                *lpNumberOfBytesWritten=i;
            SetLastError(ERROR_PARTIAL_COPY);
            return false;
        }
    }
    return true;
}

bool memisvalidreadptr(HANDLE hProcess, uint addr)
{
    unsigned char a=0;
    return memread(hProcess, (void*)addr, &a, 1, 0);
}

void* memalloc(HANDLE hProcess, uint addr, DWORD size, DWORD fdProtect)
{
    return VirtualAllocEx(hProcess, (void*)addr, size, MEM_RESERVE|MEM_COMMIT, fdProtect);
}

void memfree(HANDLE hProcess, uint addr)
{
    VirtualFreeEx(hProcess, (void*)addr, 0, MEM_RELEASE);
}

static int formathexpattern(char* string)
{
    int len=strlen(string);
    _strupr(string);
    char* new_string=(char*)emalloc(len+1, "formathexpattern:new_string");
    memset(new_string, 0, len+1);
    for(int i=0,j=0; i<len; i++)
        if(string[i]=='?' or isxdigit(string[i]))
            j+=sprintf(new_string+j, "%c", string[i]);
    strcpy(string, new_string);
    efree(new_string, "formathexpattern:new_string");
    return strlen(string);
}

static bool patterntransform(const char* text, std::vector<PATTERNBYTE>* pattern)
{
    if(!text or !pattern)
        return false;
    pattern->clear();
    int len=strlen(text);
    if(!len)
        return false;
    char* newtext=(char*)emalloc(len+2, "transformpattern:newtext");
    strcpy(newtext, text);
    len=formathexpattern(newtext);
    if(len%2) //not a multiple of 2
    {
        newtext[len]='?';
        newtext[len+1]='\0';
        len++;
    }
    PATTERNBYTE newByte;
    for(int i=0,j=0; i<len; i++)
    {
        if(newtext[i]=='?') //wildcard
        {
            newByte.n[j].all=true; //match anything
            newByte.n[j].n=0;
            j++;
        }
        else //hex
        {
            char x[2]="";
            *x=newtext[i];
            unsigned int val=0;
            sscanf(x, "%x", &val);
            newByte.n[j].all=false;
            newByte.n[j].n=val&0xF;
            j++;
        }

        if(j==2) //two nibbles = one byte
        {
            j=0;
            pattern->push_back(newByte);
        }
    }
    efree(newtext, "transformpattern:newtext");
    return true;
}

static bool patternmatchbyte(unsigned char byte, PATTERNBYTE* pbyte)
{
    unsigned char n1=(byte>>4)&0xF;
    unsigned char n2=byte&0xF;
    int matched=0;
    if(pbyte->n[0].all)
        matched++;
    else if(pbyte->n[0].n==n1)
        matched++;
    if(pbyte->n[1].all)
        matched++;
    else if(pbyte->n[1].n==n2)
        matched++;
    return (matched==2);
}

uint memfindpattern(unsigned char* data, uint size, const char* pattern, int* patternsize)
{
    std::vector<PATTERNBYTE> searchpattern;
    if(!patterntransform(pattern, &searchpattern))
        return -1;
    int searchpatternsize=searchpattern.size();
    if(patternsize)
        *patternsize=searchpatternsize;
    for(uint i=0,pos=0; i<size; i++) //search for the pattern
    {
        if(patternmatchbyte(data[i], &searchpattern.at(pos))) //check if our pattern matches the current byte
        {
            pos++;
            if(pos==searchpatternsize) //everything matched
                return i-searchpatternsize+1;
        }
        else if (pos>0) //fix by Computer_Angel
        {
            i-=pos; // return to previous byte 
            pos=0; //reset current pattern position 
        }
    }
    return -1;
}
