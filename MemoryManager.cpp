#include "MemoryManager.h"
#include <vector>
#include <iostream>
#include <climits>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

using namespace std;

Block::Block() {
    offset = 0;
    size = 0;
    allocated = false;
}

Block::Block(unsigned offset, unsigned size, bool allocated) {
    this->offset = offset;
    this->size = size;
    this->allocated = allocated;
}

MemoryManager::MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator) {
    //gives the wordSize
    this->wordSize = wordSize;
    this->memoryLimit = 0;
    this->listSize = 0;

    //need to do something with the allocator
    this->allocator = allocator;
}

MemoryManager::~MemoryManager() {
    //deletes everything in the memory
    if (memoryLimit != 0) this->shutdown();
}

void MemoryManager::initialize(size_t sizeInWords) {
    //cleans up previous block if applicable...
    if (memoryLimit != 0) this->shutdown();

    //make new node in block, all free and size of sizeInWords
    this->memoryLimit = sizeInWords * wordSize;
    Block temp(0, memoryLimit, false);
    blocks.push_back(temp);
    arr = new char[memoryLimit] ();
}

void MemoryManager::shutdown() {
    //releases memory acquired during initialization, if any
    this->memoryLimit = 0;
    this->listSize = 0;
    //deletes everything in the memory
    blocks.clear();
    delete[] arr;
}

void *MemoryManager::allocate(size_t sizeInBytes) {
    //allocated memory using the allocator function, returns nullptr if invalid size or no memory
    uint16_t* list = (uint16_t*) this->getList();
    int sizeInWords = sizeInBytes / wordSize;
    int offset;
    if (blocks[0].size == memoryLimit && !blocks[0].allocated) offset = 0;
    else offset = allocator(sizeInWords, list);
    delete[] list;
    if (offset == -1) return nullptr;
    int index = -1;
    //need to allocate memory at the offset, change whatever was previously in that block
    for (int i = 0; i < blocks.size(); i++) {
        if (blocks[i].offset == offset) {
            index = i;
            break;
        }
    }
    if (blocks[index].size == sizeInBytes) {
        blocks[index].allocated = true;
        return (void*)(arr + blocks[index].offset*wordSize);
    }
    else {
        blocks[index].size -= sizeInBytes;
        blocks[index].offset += (sizeInBytes / wordSize);
        Block allocate(offset, sizeInBytes, true);
        blocks.push_back(allocate);
        return (void*)(arr + blocks[blocks.size()-1].offset*wordSize);
    }
}

void MemoryManager::free(void *address) {
    //Frees the memory  within the memory manager so that it can be reused
    int index = -1;
    char* tempAddress = (char*) address;
    int byteOffset = tempAddress - arr;
    int realOffset = byteOffset / wordSize;
    for (int i = 0; i < blocks.size(); i++) {
        if (realOffset == blocks[i].offset) {
            index = i;
            break;
        }
    }


    //first find the blocks before and after (vector not necessarily in order)
    int nextOffset = blocks[index].offset + (blocks[index].size / wordSize);
    int nextIndex = -1;
    for (int i = 0; i < blocks.size(); i++) {
        if (blocks[i].offset == nextOffset) {
            nextIndex = i;
            break;
        }
    }
    int prevIndex = -1;
    for (int i = 0; i < blocks.size(); i++) {
        if (blocks[i].offset + (blocks[i].size / wordSize) == blocks[index].offset) {
            prevIndex = i;
            break;
        }
    }
    //now consider a couple cases when freeing
    bool first = false;
    bool last = false;
    if (prevIndex == -1) first = true;
    if (nextIndex == -1) last = true;

    //if first
    if (first) {
        //if next block is free
        if (!blocks[nextIndex].allocated) {
            //combine to one hole
            unsigned offset = blocks[index].offset;
            unsigned size = blocks[nextIndex].size + blocks[index].size;
            Block hole(offset, size, false);
            int deleteFirst = max(nextIndex, index);
            int deleteLast = min(nextIndex, index);
            blocks.erase(blocks.begin() + deleteFirst);
            blocks.erase(blocks.begin() + deleteLast);
            blocks.push_back(hole);
        }
        else {
            blocks[index].allocated = false;
        }
    }

    //if last
    else if (last) {
        if (!blocks[prevIndex].allocated) {
            //combine to one hole
            unsigned offset = blocks[prevIndex].offset;
            unsigned size = blocks[prevIndex].size + blocks[index].size;
            Block hole(offset, size, false);
            int deleteFirst = max(prevIndex, index);
            int deleteLast = min(prevIndex, index);
            blocks.erase(blocks.begin() + deleteFirst);
            blocks.erase(blocks.begin() + deleteLast);
            blocks.push_back(hole);
        }
        else {
            blocks[index].allocated = false;
        }
    }

    //if both block before and after are free:
    else if (!blocks[prevIndex].allocated && !blocks[nextIndex].allocated) {
        //combine into one big hole
        unsigned tempOffset = blocks[prevIndex].offset;
        unsigned tempSize = blocks[prevIndex].size + blocks[index].size + blocks[nextIndex].size;
        Block hole(tempOffset, tempSize, false);
        vector<int> sorted{nextIndex, index, prevIndex};
        sort(sorted.begin(), sorted.end());
        blocks.erase(blocks.begin() + sorted[2]);
        blocks.erase(blocks.begin() + sorted[1]);
        blocks.erase(blocks.begin() + sorted[0]);
        blocks.push_back(hole);
    }

    //if only the block before is free:
    else if (!blocks[prevIndex].allocated) {
        //combine to one hole
        unsigned offset = blocks[prevIndex].offset;
        unsigned size = blocks[prevIndex].size + blocks[index].size;
        Block hole(offset, size, false);
        int deleteFirst = max(prevIndex, index);
        int deleteLast = min(prevIndex, index);
        blocks.erase(blocks.begin() + deleteFirst);
        blocks.erase(blocks.begin() + deleteLast);
        blocks.push_back(hole);
    }

    //if only the block after is free:
    else if (!blocks[nextIndex].allocated) {
        //combine to one hole
        unsigned offset = blocks[index].offset;
        unsigned size = blocks[nextIndex].size + blocks[index].size;
        Block hole(offset, size, false);
        int deleteFirst = max(nextIndex, index);
        int deleteLast = min(nextIndex, index);
        blocks.erase(blocks.begin() + deleteFirst);
        blocks.erase(blocks.begin() + deleteLast);
        blocks.push_back(hole);
    }

    //if both the surrounding blocks are not free:
    else if (blocks[prevIndex].allocated && blocks[nextIndex].allocated) {
        blocks[index].allocated = false;
    }
}

void MemoryManager::setAllocator(std::function<int(int, void *)> allocator) {
    //changes the allocation algorithm to identifying the memory hole to use for allocation
    this->allocator = allocator;
}

int MemoryManager::dumpMemoryMap(char *filename) {
    //uses standard POSIX calls to write hole list to filename as text
    int file;
    file = open(filename, O_CREAT|O_WRONLY, 0666);
    if (file == -1) return -1;
    char buf[1024];
    string temp = "";
    uint16_t* list = (uint16_t*) this->getList();
    if (!list) {
        close(file);
        return 0;
    }
    if (list[0] == 0) {
        delete[] list;
        close(file);
        return 0;
    }
    temp += "[";
    temp += to_string(list[1]);
    for (int i = 2; i < list[0]*2; i+=2) {
        temp += ", ";
        temp += to_string(list[i]);
        temp += "] - [";
        temp += to_string(list[i+1]);
    }
    temp += ", ";
    temp += to_string(list[list[0]*2]);
    temp += "]";
    strcpy(buf, temp.c_str());
    int err;
    err = write(file, buf, strlen(buf));
    delete[] list;
    close(file);
    if (err == -1) return -1;
    return 0;
}

void *MemoryManager::getList() {
    //returns an array of information (in decimal) about holes for use by the allocator function
    int holeCount = 0;
    for (int i = 0; i < blocks.size(); i++) {
        if (!blocks[i].allocated) holeCount++;
    }
    if (holeCount == 1 && blocks[0].size == memoryLimit) {
        listSize = 0;
        return nullptr;
    }
    listSize = (holeCount*2+1);
    uint16_t* arrTemp = new uint16_t[listSize];
    vector<pair<int, int>> helper;
    for (int i = 0; i < blocks.size(); i++) {
        if (!blocks[i].allocated) {
            pair<int, int> temp(blocks[i].offset, blocks[i].size / wordSize);
            helper.push_back(temp);
        }
    }
    sort(helper.begin(), helper.end());
    arrTemp[0] = holeCount;
    int index = 1;
    for (int i = 0; i < helper.size(); i++) {
        arrTemp[index] = helper[i].first;
        arrTemp[index+1] = helper[i].second;
        index+=2;
    }
    return arrTemp;
}

void *MemoryManager::getBitmap() {
    //returns a bit-stream of bits in terms of an array representing whether words are used or free
    uint16_t* list = (uint16_t*) this->getList();
    int sizeInWords = memoryLimit / wordSize;
    vector<int> littleEndian(sizeInWords, 1);
    for (int i = 1; i < listSize; i+=2) {
        int tempOffset = list[i];
        int tempSize = list[i+1];
        for (int j = 0; j < tempSize; j++) {
            littleEndian[littleEndian.size()-1-tempOffset-j] = 0;
        }
    }
    //now once we have the little Endian vector filled, convert to BitMap
    int tempSize;
    if (littleEndian.size() % 8 == 0) tempSize = littleEndian.size() / 8;
    else tempSize = (littleEndian.size() / 8) + 1;
    uint8_t* toReturn = new uint8_t[tempSize + 2];

    //convert our size to lowByte highByte hex strings and use those to fill bitMap size
    stringstream ss;
    ss << hex << setfill('0');
    ss << setw(4) << tempSize;
    string hexValue(ss.str());
    string lowByte = hexValue.substr(hexValue.length() - 2);
    string highByte = hexValue.substr(0, 2);
    int lowInt;
    int highInt;
    stringstream x;
    x << lowByte;
    x >> hex >> lowInt;
    stringstream y;
    y << highByte;
    y >> hex >> highInt;
    
    toReturn[0] = lowInt;
    toReturn[1] = highInt;

    //for loop through all the bytes and convert from binary to decimal
    string tempByte = "";
    int counter = 0;
    int remainder = littleEndian.size() % 8;
    for (int i = 0; i < remainder; i++) {
        tempByte += to_string(littleEndian[i]);
    }
    if (remainder != 0) {
        int r = stoi(tempByte, nullptr, 2);
        tempByte = "";
        toReturn[tempSize + 1] = r;
    }
    for (int i = remainder; i < littleEndian.size(); i++) {
        if (counter == 8) {
            int tempInt = stoi(tempByte, nullptr, 2);
            if (remainder == 0) toReturn[tempSize + 2 - i/8] = tempInt;
            else toReturn[tempSize + 1 - i/8] = tempInt;
            tempByte = "";
            counter = 0;
        }
        tempByte += to_string(littleEndian[i]);
        counter++;
    }
    int tempInt = stoi(tempByte, nullptr, 2);
    toReturn[2] = tempInt;
    delete[] list;
    return toReturn;
}

unsigned MemoryManager::getWordSize() {
    return this->wordSize;
}

void *MemoryManager::getMemoryStart() {
    //returns the byte-wise memory address of the beginning of the memory
    return arr;
}

unsigned MemoryManager::getMemoryLimit() {
    //returns the byte limit of the current memory
    return this->memoryLimit;
}

int bestFit(int sizeInWords, void *list) {
    //returns word offset of hole selected by best fit memory allocation algorithm
    uint16_t* tempList = (uint16_t*) list;
    uint16_t listSizeTemp = tempList[0];
    int difference = INT_MAX;
    int offset = -1;
    for (int i = 2; i < listSizeTemp*2+1; i+=2) {
        if (tempList[i] - sizeInWords < difference && tempList[i] - sizeInWords >= 0) {
            difference = tempList[i] - sizeInWords;
            offset = tempList[i-1];
        }
    }
    return offset;
}

int worstFit(int sizeInWords, void *list) {
    //returns word offset of hole selected by worst fit memory allocation algorithm
    uint16_t* tempList = (uint16_t*) list;
    uint16_t listSizeTemp = tempList[0];
    int difference = -1;
    int offset = -1;
    for (int i = 2; i < listSizeTemp*2+1; i+=2) {
        if (tempList[i] - sizeInWords > difference && tempList[i] - sizeInWords >= 0) {
            difference = tempList[i] - sizeInWords;
            offset = tempList[i-1];
        }
    }
    return offset;
}

