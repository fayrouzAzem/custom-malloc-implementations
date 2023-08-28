#include <unistd.h>
#include <cstring>
#define MAX 1e8

struct MallocMetadata 
{
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

MallocMetadata* global_head = nullptr;

MallocMetadata* searchForAFreeBlock(size_t size) 
{
    MallocMetadata* current_block = global_head;

    while (current_block != nullptr) {
        if (current_block->is_free && current_block->size >= size) {
            return current_block;
        }
        current_block = current_block->next;
    }
    return nullptr;
}

MallocMetadata* getLastBlockInTheList () 
{
    MallocMetadata* current_block = global_head;

    while (current_block->next != nullptr) {
        current_block = current_block->next;
    }
    return current_block;
}

void* smalloc(size_t size) 
{
    if (size == 0 || size > MAX) {
        return nullptr;
    }

    MallocMetadata* found_block = searchForAFreeBlock(size);

    if (found_block == nullptr) {
        MallocMetadata* new_block = (MallocMetadata*)sbrk(size + sizeof(MallocMetadata));
        if ((void*)new_block == (void*)(-1)) {
            return nullptr;
        }
        new_block->size = size;
        new_block->is_free = false;
        if (global_head == nullptr) {
            global_head = new_block;
            new_block->next = nullptr;
            new_block->prev = nullptr;
        } else {							
            MallocMetadata* last_block = getLastBlockInTheList();
            last_block->next = new_block;
            new_block->prev = last_block;
            new_block->next = nullptr;
        }
        return (void*)(new_block + 1);
    }
    found_block->is_free = false;
    return (void*)(found_block + 1);
}

void* scalloc(size_t num, size_t size) 
{
    void* ptr = smalloc(num * size);

    if (ptr == nullptr) {
        return nullptr;
    }
    memset(ptr, 0, num * size);
    return ptr;
}

void sfree(void* p) 
{
    if (p == nullptr) {
        return;
    }

    MallocMetadata* block_to_free = (MallocMetadata*)p - 1;

    if (block_to_free->is_free == true) {
        return;
    }
    block_to_free->is_free = true;
}

void* srealloc(void* oldp, size_t size) 
{
    if (size == 0 || size > MAX) {
        return nullptr;
    }
    if (oldp == nullptr) {
        return smalloc(size);
    }

    MallocMetadata* block_to_realloc = (MallocMetadata*)oldp - 1;

    if (block_to_realloc->size >= size) {
        return oldp;
    }
    void* newp = smalloc(size);
    if (newp == nullptr) {
        return nullptr;
    }
    memmove(newp, oldp, block_to_realloc->size);
    sfree(oldp);
    return newp;
}

size_t _num_free_blocks() 
{
    size_t num_free_blocks = 0;
    MallocMetadata* current_block = global_head;

    while (current_block != nullptr) {
        if (current_block->is_free == true) {
            num_free_blocks++;
        }
        current_block = current_block->next;
    }
    return num_free_blocks;
}

size_t _num_free_bytes() 
{
    size_t num_free_bytes = 0;
    MallocMetadata* current_block = global_head;

    while (current_block != nullptr) {
        if (current_block->is_free == true) {
            num_free_bytes += current_block->size;
        }
        current_block = current_block->next;
    }
    return num_free_bytes;
}

size_t _num_allocated_blocks() 
{
    size_t num_allocated_blocks = 0;
    MallocMetadata* current_block = global_head;

    while (current_block != nullptr) {
        num_allocated_blocks++;
        current_block = current_block->next;
    }
    return num_allocated_blocks;
}

size_t _num_allocated_bytes() 
{
    size_t num_allocated_bytes = 0;
    MallocMetadata* current_block = global_head;

    while (current_block != nullptr) {
        num_allocated_bytes += current_block->size;
        current_block = current_block->next;
    }
    return num_allocated_bytes;
}

size_t _num_meta_data_bytes() 
{
    return _num_allocated_blocks() * sizeof(MallocMetadata);
}

size_t _size_meta_data() 
{
    return sizeof(MallocMetadata);
}