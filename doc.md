# Memory Segments in Usual Applications

Typically, a typical application running in a modern operating system environment divides its memory into several segments or regions, each serving unique roles. The main memory segments include:

1. **Text Segment (Code Segment)**: 
   - Contains the compiled code of the program, which is executed by the CPU. It is usually marked as read-only to prevent the program from accidentally modifying its instructions.

2. **Data Segment**: 
   - Divided into initialized and uninitialized parts.
   - **Initialized Data Segment (.data)**: Contains global and static variables that are initialized by the programmer.
   - **Uninitialized Data Segment (BSS - Block Started by Symbol)**: Includes global and static variables that are not initialized by the programmer. At runtime, the operating system initializes them to zero.

3. **Heap Segment**:
   - Managed with system calls like `malloc` in C or `new` in C++ for dynamic memory allocation during the application's runtime. Used for variables whose size is not known at compile time and can grow as needed at runtime until the memory is exhausted or freed.

4. **Stack Segment**:
   - Automatically managed memory that stores local variables, function parameters, return addresses, and the function call stack. It supports the LIFO (Last In, First Out) method to keep track of function invocations and local variables.

5. **Command-line Arguments and Environment Variables**:
   - Memory segments that store command-line arguments and environment variables accessible by the program.

### Default Size of the Stack Segment

The default size of the stack segment can vary significantly depending on the operating system, whether it's a desktop, server, or different platform like Windows, Linux, or macOS.

- **On 64-bit Linux**, the default stack size is typically around 8 MB. This value can be checked and modified with the `ulimit` command (`ulimit -s` to view and `ulimit -s <size>` to set).
- **On Windows**, the default stack size is specified in the executable file header and can be set during linking via the `/STACK` linker option. Visual Studio sets this at 1 MB by default for 32-bit and 64-bit applications.

### Natural Alignment

Natural alignment refers to an object being aligned in memory at byte addresses that are multiples of its size. For instance, a type requiring `N` bytes should be placed at an address that is a multiple of `N`:

- An `int` on a system where integers are 4 bytes should be placed at memory addresses like `... 0, 4, 8, 12, 16, ...`
- A `double` typically takes 8 bytes and should be aligned on addresses that are multiples of 8, such as `... 0, 8, 16, 24, 32, ...`

Natural alignment is important because it allows the CPU to process data more efficiently. Many architectures require this type of alignment and accessing data that is not naturally aligned may result in slower performance or, on strict alignment architectures, could lead to runtime faults (like alignment faults). For modern applications and compilers, alignment is automatically managed, but it’s important for low-level and performance-critical programming.

# Exception Handling

## Materials

### Find yourself and read at least 3 different sources about exception handling in C++:

Investigated docs:

1. [Exception Handling in C++](https://www.geeksforgeeks.org/exception-handling-c/)

1. [Modern C++ best practices for exceptions and error handling](https://learn.microsoft.com/en-us/cpp/cpp/errors-and-exception-handling-modern-cpp?view=msvc-170)

1. [How to: Interface between exceptional and non-exceptional code](https://learn.microsoft.com/en-us/cpp/cpp/how-to-interface-between-exceptional-and-non-exceptional-code?view=msvc-170)

1. [C++ Exception Handling](https://www.tutorialspoint.com/cplusplus/cpp_exceptions_handling.htm)

1. [C++ Exceptions](https://www.w3schools.com/cpp/cpp_exceptions.asp)

1. [C++ Exception Handling](https://www.programiz.com/cpp-programming/exception-handling)

### Investigate about levels of exception safety guaranties in C++.

Investigated docs:

1. [Levels of Exception Safety](https://arne-mertz.de/2015/12/levels-of-exception-safety/)

1. [Exception Safety and RAII (Resource Acquisition Is Initialization) in C++](https://www.w3computing.com/articles/exception-safety-raii-cpp/)

1. [Exception Safety in C++ - A Comprehensive Guide](https://www.surfsidemedia.in/post/exception-safety-in-c-plus-a-comprehensive-guide)

### Read from link below as well as all nearby links at the same level of the documentation

1. [Exceptions and Stack Unwinding in C++](https://learn.microsoft.com/en-us/cpp/cpp/exceptions-and-stack-unwinding-in-cpp?view=msvc-170)

1. [Exception handling](https://eel.is/c++draft/except.throw)

### Questions

[Source](https://learn.microsoft.com/en-us/cpp/cpp/how-to-design-for-exception-safety?view=msvc-170#exception-safe-classes)

Consider whether to store all class state in a data member that is wrapped in a smart pointer, especially if a class has a concept of "initialization that is permitted to fail." Although C++ allows for uninitialized data members, it does not support uninitialized or partially initialized class instances. A constructor must either succeed or fail; no object is created if the constructor does not run to completion.

## Task

### Modify application from Module "Multiprocessing Communication":

1. Implement own terminate function to close at any circumstances shared memory handle as well as opened for reading/writing file - Done
     
1. Add throw any exception code after complete of first memory block reading/writing (take care about "to close" another process) - In progress

1. Inject some classes in logic for logging purposes (constructor + destructor) - Done



