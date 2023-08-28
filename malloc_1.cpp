#include <unistd.h>
#define MAX 1e8

void* smalloc(size_t size) {
    if (size == 0 || size > MAX) {
        return nullptr;
    }
    void* ptr = sbrk(size);
    if (ptr == (void*)(-1)) {
        return nullptr;
    }
    return ptr;
}


