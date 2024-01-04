# uC-BASIC
Embeddable BASIC interpreter for use in microcontrollers
# Features
- Fully implements the features and syntax of Altair (R) BASIC 3.2 (4K version)
- Written in plain C99, no dependency on OS or hardware
- Embeds into user projects just by providing I/O callbacks. The interpreter can be called to execute a command, or to run an interactive command prompt
- Fully object-oriented. The interpreter state can be allocated by the user in any way he likes: statically, dynamically, or on the stack. You can have multiple instances of uC-BASIC running simultaneously in your system given OS support
- Optimized for low RAM and stack usage
- Bounded stack usage - does not use recursive function calls
- Bounded RAM usage - uses only the user-specified amount of RAM for storing the program, its variables, and FOR/GOSUB stack
- The internal program representation is tokenized to save memory. The program format is exactly the same as on Altair (R) BASIC 3.2 (4K)
- Does not use dynamic memory allocation. No malloc. No heap fragmentation
- Does not need mutexes or other kinds of lock. Suitable for use in real-time systems
- Small code footprint - about 12K on an STM32 MCU
- Thoroughly tested. A comprehensive test suite is provided.
- High code quality: No compiler warnings with standard GCC settings
- Fast: @1MHz STM32F412 faster than most classic BASICs
- An example port is provided for NUCLEO-F412ZG board
# Speed
The results of the Rugg/Feldman benchmarks (https://en.wikipedia.org/wiki/Rugg/Feldman_benchmarks)
when running on an STM32F412 @ 1MHz clock frequency are given below.
- Benchmark 1: 0.427s
- Benchmark 2: 3.480s
- Benchmark 3: 6.340s
- Benchmark 4: 6.956s
- Benchmark 5: 7.724s
- Benchmark 6: 12.008s
- Benchmark 7: 20.108s
- Benchmark 8: not evaluated (no support for respective functions yet)

Evaluation:
uC-BASIC exceeds the performance of most classic BASIC interpreters of the '70s and '80s era, except few (ABC 80, ABC 800, in one case Apple ][ and BBC Micro).

It was attempted to make the speed tests fair by setting the clock frequency of the CPU to 1MHz, which is far below its maximum capability. On one hand, the uC-BASIC takes advantage of the modern ARM CPU architecture, 32-bit CPU, and an FPU. On the other hand, the uC-BASIC suffers the penalty of being written in C, rather than Assembler for classic BASICs, missing numerous optimization opportunities. Also the software stack implementation of expression parsing, which ensures bounded machine stack usage, has its price.

All in all, it's a good starting point. It is confirmed that this BASIC implementation is competitive. The interpreter could be further optimized for performance, but I didn't really try it hard yet.
