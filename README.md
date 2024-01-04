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
- An example port is provided for NUCLEO-F412ZG board
