#include <windows.h>
#include <stdio.h>

typedef bool (*CBRET)();

unsigned int globalvar = 0;

int main()
{
    unsigned int lol;
    unsigned char* page = (unsigned char*)VirtualAlloc(0, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if(!page)
    {
        puts("error: VirtualAlloc");
        return 1;
    }
    page[0] = 0x6A;
    puts("write");
    page[1] = 0x01;
    puts("write");
    page[2] = 0x58;
    puts("write");
    page[3] = 0xC3;
    puts("write");
    CBRET cb = (CBRET)page;
    cb();
    puts("exec");
    lol = globalvar;
    puts("read");
    lol = page[1];
    puts("read");
    lol = page[3];
    puts("read");
    lol = page[2];
    puts("read");
    return 0;
}
