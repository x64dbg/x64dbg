// Linked static and static-pie, so no ld.so or shared libc is mapped.
#include <unistd.h>

volatile int st_ready = 0;
char st_bss[64 * 1024];

int main(void)
{
    st_bss[0] = 1;
    st_ready = 1;
    for(;;)
        pause();
}
