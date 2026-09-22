#include <unistd.h>

extern "C" void ep_site();

asm(R"(
    .text

    .globl ep_site
    .type  ep_site, @function
ep_site:
    ret
)");

int main(int argc, char** argv)
{
    ep_site();

    if(argc > 1)
    {
        char* peer[] = {argv[1], nullptr};
        execv(argv[1], peer);
        return 1;
    }

    return 9;
}
