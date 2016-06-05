# AMTW3-Context-Switching

The code for the Advanced Microcontroller Topics Workshop #3: Context Switching

This project contains the implementation of a simple preemptively-multi-threaded operating system. A simple kernel and threading model are implemented and documented in ```kernel.h```, ```kernel.c```, ```thread.h```, and ```thread.c```. The kernel entry and exit routines, as well as the system call stubs, are implemented in assembly in ```kernel_asm.S```. The kernel entry and exit routines are not complete, and must be implemented by the participants. A complete, functional implementation can be found in ```kernel_asm.S.reference``` for reference. A simple 3-thread program is implemented in ```main.c``` which blinks the onboard R, G, and B leds at different rates.

The presentation slides that accompany this code can be viewed [here](https://docs.google.com/presentation/d/1_H9AfzI-TKpd0Ppy_6LTWpOVqkkSqrGKujjAkqGLbuY/edit?usp=sharing).
