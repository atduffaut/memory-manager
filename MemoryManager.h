#pragma once
#include <functional>
#include <list>
#include <vector>

using namespace std;

int bestFit(int sizeInWords, void *list);

int worstFit(int sizeInWords, void *list);

struct Block {
    unsigned offset;
    unsigned size;
    bool allocated;
    Block();
    Block(unsigned offset, unsigned size, bool free);
};

class MemoryManager {
    private:
    unsigned wordSize;
    unsigned memoryLimit;
    vector<Block> blocks;
    char* arr;
    unsigned listSize;
    std::function<int(int, void *)> allocator;
    public:
    MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator);

    ~MemoryManager();

    void initialize(size_t sizeInWords);

    void shutdown();

    void *allocate(size_t sizeInBytes);

    void free(void *address);

    void setAllocator(std::function<int(int, void *)> allocator);

    int dumpMemoryMap(char *filename);

    void *getList();

    void *getBitmap();

    unsigned getWordSize();

    void *getMemoryStart();

    unsigned getMemoryLimit();
};
