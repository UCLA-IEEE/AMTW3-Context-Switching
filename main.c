#include "driverlib/sysctl.h"
#include "drivers/driver_serial.h"
#include "kernel.h"
#include "syscalls.h"
#include <stdlib.h>

lock_t printlock = 0;

int worker1_main(void* arg)
{
    while(1)
    {
        while(!sys_lock(&printlock));
        sys_sleep(1000);
        Serial_puts(Serial_module_debug, "Working on 1!\r\n");
        sys_unlock(&printlock);
    }
}

int worker2_main(void* arg)
{
    while(1)
    {
        while(!sys_lock(&printlock));
        sys_sleep(500);
        Serial_puts(Serial_module_debug, "Working on 2!\r\n");
        sys_unlock(&printlock);
    }
}

int main(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ
                   | SYSCTL_OSC_MAIN);

    Serial_init(Serial_module_debug, 115200);

    Serial_puts(Serial_module_debug, "Hello, world!\r\n");

    kernel_init(kernel_stack + sizeof(kernel_stack));

    sys_spawn(worker1_main, NULL);
    sys_spawn(worker2_main, NULL);

    while(1)
    {
        sys_yield();
    }

    return 0;
}
