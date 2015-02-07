#include <stdio.h>
#include <cstring>
#include <stdint.h>

#define uint size_t
#define PAGE_SIZE 0x1000

#ifdef _WIN64
#define HIGHEST_USER_ADDR 0x7FFFFFEFFFF
#else //x86
#define HIGHEST_USER_ADDR 0x7FFEFFFF
#endif // _WIN64

bool readblock(uint addr, unsigned char block[PAGE_SIZE])
{
    printf("readblock(%X[%X])\n", addr, PAGE_SIZE);
    memset(block, 0xFF, PAGE_SIZE);
    return true;
}

bool memread(uint addr, unsigned char* data, uint size)
{
    //check if the address is inside user space
    if(addr > HIGHEST_USER_ADDR)
        return false;

    puts("-start-");
    printf("  addr: %X\n  size: %X\n", addr, size);

    //calculate the start page
    uint start = addr & ~(PAGE_SIZE - 1);
    printf(" start: %X\n", start);

    //calculate the end page
    uint end = addr + size;
    uint x = end & (PAGE_SIZE - 1);
    if(x)
        end += (PAGE_SIZE - x);
    printf("   end: %X\n", end);

    //calculate the number of pages to read
    uint npages = (end - start) / PAGE_SIZE;
    printf("npages: %d\n\n", npages);

    //go over all pages
    for(uint i = 0, j = start; i < npages; i++)
    {
        //read one page (j should always align with PAGE_SIZE)
        unsigned char block[PAGE_SIZE];
        if(!readblock(j, block))
        {
            return false;
        }

        //these are the offsets and sizes in the block to write to append to the output buffer
        uint roffset = 0;
        uint rsize = PAGE_SIZE;

        if(i == npages - 1) //last page (first because there might only be one page)
        {
            rsize = size - (j - start); //remaining size
        }
        else if(i == 0) //first page
        {
            roffset = addr & (PAGE_SIZE - 1);
            rsize = PAGE_SIZE - roffset;
        }

        printf("roffset: %X\n  rsize: %X\n", roffset, rsize);
        puts("");

        //copy the required block data in the output buffer
        memcpy(data, block + roffset, rsize);
        data += rsize;

        j += rsize;
    }

    puts("--end--\n");
    return true;
}

int main()
{
    unsigned char out[0x10000] = {0};
    memread(0x12A45, out, 0x3456);
    memread(0x12000, out, 0x456);
    memread(0x12000, out, 0x3456);
    memread(0x12000, out, 0x4000);
    memread(0x12ff0, out, 0x16);
    memread(0x100, out, 0x3090);
    return 0;
}