# OS memory-manager
This is an implementation of an OS memory-manager built in C++. To build the system library, simply call `make` (the caveat is that you have to be using a Unix system since this was built using Unix syscalls).

Memory managers in fully fleshed out operating systems like Linux have complicated algorithms to swap, free, and allocate memory. For this memory manager, I've only implemented the Best Fit and Worst Fit algorithms (although adding more algorithms in the future is not an impossibility and should be easy to tack on). You can read more about memory managers [here](https://www.geeksforgeeks.org/memory-management-in-operating-system/).

The memory manager built here has the ability to free and allocate new memory, and it takes care of all the specifics of how that happens based on the chosen fitting algorithm (either best fit or worst fit). At any time in using the manager, you have the ability to request what the current memory block looks like using either getList() or getBitmap() depending on what you want the data to look like. This visibility should show you how the blocks of memory are being allocated and freed, and give you a little more insight into how a basic memory manager works :)

An example testing script for the memory manager is provided (CommandLineTest.cpp) and it shows you the different ways you can use the memory manager library to see how an Operating System works with memory!
