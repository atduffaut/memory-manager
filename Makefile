build_library:
	cc -c MemoryManager.cpp
	ar cr libMemoryManager.a MemoryManager.o
