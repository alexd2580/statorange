#define _XOPEN_SOURCE

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

volatile sig_atomic_t x = 0;

void sig(int s)
{
    (void)s;
    x++;
    if (x > 5)
    {
        exit(2);
    }
}

int main(void)
{
    struct sigaction sa;
    sa.sa_handler = sig;               
    sigemptyset(&sa.sa_mask);
    //sa.sa_flags = SA_RESTART | SA_NODEFER;
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    for(;;);
    return 0;
}
