#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

volatile sig_atomic_t x = 0;

void sig(int s)
{
    x++;
    if (x > 5)
    {
        exit(2);
    }
}

int main(void)
{
    signal(SIGINT, sig);
    for(;;);
    return 0;
}
