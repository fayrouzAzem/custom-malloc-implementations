#include <unistd.h>
#include <sys/mman.h>
#include <cstring>
#include <cstdlib>
#include <cmath>

#define MAX 100000000
#define MAX_ORDER 10
#define KB 1024
#define STARTING_BLOCKS_NUM 32
#define MIN_KB_BLOCK_SIZE 128
#define LARGE_BLOCKS_THRESHOLD 128 * 1024

struct MallocMetadata {
	int cookie;
	size_t size;
	int order;
	bool is_free;
	MallocMetadata * next;
	MallocMetadata * prev;
	MallocMetadata * next_free;
	MallocMetadata * prev_free;
};

size_t blocks;
MallocMetadata * array_of_lists[MAX_ORDER + 1];
MallocMetadata * mmap_list = nullptr;
MallocMetadata * start;
MallocMetadata * end;
int COOKIE_VALUE = rand();
bool start_address = false;

void alignBlocks() 
{
	size_t program_break = (size_t)sbrk(0);
	size_t remainder = STARTING_BLOCKS_NUM * LARGE_BLOCKS_THRESHOLD;
	size_t shift = remainder - (program_break % remainder);

	sbrk(shift);
	int i = 0;
	blocks = (size_t)sbrk(32 * LARGE_BLOCKS_THRESHOLD);
	while (i < STARTING_BLOCKS_NUM) {
		MallocMetadata * newMetaData = (MallocMetadata *)(blocks + i * LARGE_BLOCKS_THRESHOLD);
		newMetaData->size = LARGE_BLOCKS_THRESHOLD - sizeof(MallocMetadata);
		newMetaData->is_free = true;
		newMetaData->next = nullptr;
		newMetaData->prev = nullptr;
		newMetaData->order = MAX_ORDER;
		newMetaData->next_free = nullptr;
		newMetaData->prev_free = nullptr;
		newMetaData->cookie = COOKIE_VALUE;
		if (i == 0) {
			start = newMetaData;
		}
		if (i == STARTING_BLOCKS_NUM - 1) {
			end = newMetaData;
		}
		i++;
	}
	i = 0;
	while (i < STARTING_BLOCKS_NUM) {
		MallocMetadata * newMetaData = (MallocMetadata *)(blocks + i * LARGE_BLOCKS_THRESHOLD);
		if (i < STARTING_BLOCKS_NUM - 1) {
			newMetaData->next = (MallocMetadata *)(blocks + (i + 1) * LARGE_BLOCKS_THRESHOLD);
		}
		if (i > 0) {
			newMetaData->prev = (MallocMetadata *)(blocks + (i - 1) * LARGE_BLOCKS_THRESHOLD);
		}
		i++;
	}
}
 
MallocMetadata * searchForAFreeBlock(size_t data_size) 
{
	int order = 0;
	size_t curr_size = MIN_KB_BLOCK_SIZE;
	size_t new_size = MIN_KB_BLOCK_SIZE - sizeof(MallocMetadata);

	while ((new_size < data_size || array_of_lists[order] == nullptr) && order <= MAX_ORDER) {
		order++;
		curr_size = curr_size * 2;
		new_size = curr_size - sizeof(MallocMetadata);
	}
	return array_of_lists[order];
}

void add_free(MallocMetadata * data) 
{
	if (!data) {
		return;
	}
	data->is_free = true;

	MallocMetadata * start = array_of_lists[data->order];

	if (!start) {
		array_of_lists[data->order] = data;
		return;
	}
	if ((size_t)data < (size_t)start) {
		data->next_free = start;
		start->prev_free = data;
		array_of_lists[data->order] = data;
		return;
	}
	while (start) {
		if ((size_t)data > (size_t) start) {
			if (!start->next_free) {
				start->next_free = data;
				data->prev_free = start;
				return;
			}
			if ((size_t)data < (size_t)start->next_free) {
				data->prev_free = start;
				data->next_free = start->next_free;
				start->next_free->prev_free = data;
				start->next_free = data;
				return;
			}
		}
		start = start->next_free;
	}
}

MallocMetadata * splitBlock(MallocMetadata * data, size_t size) 
{
	if (!data) {
		return nullptr;
	}
	if (size > data->size) {
		return nullptr;
	}
	if (data->cookie != COOKIE_VALUE) {
		exit(0xdeadbeef);
	}

	size_t halved_size = ((data->size + sizeof(MallocMetadata)) / 2) - sizeof(MallocMetadata);

	while (size <= halved_size && data->size > MIN_KB_BLOCK_SIZE) {
		if (!data || data->order == 0) {
			break;
		}
		if (data->is_free) {
			if (array_of_lists[data->order] == data) {
				array_of_lists[data->order] = data->next_free;
			}
			if (data->prev_free) {
				data->prev_free->next_free = data->next_free;
			}
			if (data->next_free) {
				data->next_free->prev_free = data->prev_free;
			}
			data->next_free = nullptr;
			data->prev_free = nullptr;

		}
		size_t size_before_split = data->size + sizeof(MallocMetadata);
		size_t new_size = size_before_split / 2 - sizeof(MallocMetadata);
		MallocMetadata * new_data = (MallocMetadata *)((size_t)data + size_before_split / 2);
		data->size = new_size;
		data->order--;

		new_data->size = new_size;
		new_data->order = data->order;
		new_data->next = data->next;
		new_data->prev = data;
		new_data->cookie = COOKIE_VALUE;
		new_data->is_free = true;
		data->next = new_data;

		add_free(new_data);

		if (data->is_free) {
			add_free(data);
		}
		if (new_data == nullptr) {
			break;
		}
		halved_size = ((data->size + sizeof(MallocMetadata)) / 2) - sizeof(MallocMetadata);
	}
	return data;
}

void mergeBlocks(MallocMetadata * block) 
{
	if (block->cookie != COOKIE_VALUE) {
		exit(0xdeadbeef);
	}

	size_t size = block->size + sizeof(MallocMetadata);
	MallocMetadata * buddyPart = (MallocMetadata *)(((size_t)block) ^ size); // Xoring

	if (buddyPart->cookie != COOKIE_VALUE) {
		exit(0xdeadbeef);
	}
	while (size < LARGE_BLOCKS_THRESHOLD && block->is_free && buddyPart->is_free) {
		if (block->order == MAX_ORDER || buddyPart->order == MAX_ORDER || block->size != buddyPart->size) {
			break;
		}
		if (block->next != buddyPart && buddyPart->next != block) {
			break;
		}
		if ((size_t)block > (size_t) buddyPart) {
			MallocMetadata * now = block;
			block = buddyPart;
			buddyPart = now;
		}
		if (block->is_free) {
			if (block->cookie != COOKIE_VALUE) {
				exit(0xdeadbeef);
			}
			if (array_of_lists[block->order] == block) {
				array_of_lists[block->order] = block->next_free;
			}
			if (block->prev_free) {
				block->prev_free->next_free = block->next_free;
			}
			if (block->next_free) {
				block->next_free->prev_free = block->prev_free;
			}
			block->next_free = nullptr;
			block->prev_free = nullptr;
		}
		if (buddyPart->is_free) {
			if (buddyPart->cookie != COOKIE_VALUE) {
				exit(0xdeadbeef);
			}
			if (array_of_lists[buddyPart->order] == buddyPart) {
				array_of_lists[buddyPart->order] = buddyPart->next_free;
			}

			if (buddyPart->prev_free) {
				buddyPart->prev_free->next_free = buddyPart->next_free;
			}

			if (buddyPart->next_free) {
				buddyPart->next_free->prev_free = buddyPart->prev_free;
			}

			buddyPart->next_free = nullptr;
			buddyPart->prev_free = nullptr;
			buddyPart->is_free = false;
		}
		block->next = buddyPart->next;
		if (buddyPart->next) {
			buddyPart->next->prev = block;
		}
		size_t orig_block_size = block->size + sizeof(MallocMetadata);
		size_t new_block_size = orig_block_size * 2;
		size_t new_size = new_block_size - sizeof(MallocMetadata);
		block->size = new_size;
		block->order++;
		if (block->is_free) {
			add_free(block);
		}
		auto currento_sizo = block->size + sizeof(MallocMetadata);
		buddyPart = (MallocMetadata *)(((size_t)block) ^ currento_sizo);

		size = block->size + sizeof(MallocMetadata);
	}
}

void * smalloc(size_t size) 
{
	if (!start_address) {
		alignBlocks();
		for (int i = 0; i <= MAX_ORDER; i++) {
			array_of_lists[i] = nullptr;
		}
		array_of_lists[MAX_ORDER] = start;
		MallocMetadata * start_of_free_list = start;

		while (start_of_free_list) {
			start_of_free_list->next_free = start_of_free_list->next;
			start_of_free_list->prev_free = start_of_free_list->prev;
			start_of_free_list = start_of_free_list->next;
		}
	}
	start_address = true;
	if (size == 0 || size > MAX) {
		return nullptr;
	}
	if (size > LARGE_BLOCKS_THRESHOLD) { 
		MallocMetadata * new_block = (MallocMetadata *)mmap(NULL, size + sizeof(MallocMetadata), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		new_block->size = size;
		new_block->is_free = false;
		new_block->order = -1; 
		new_block->cookie = COOKIE_VALUE;
		if (!mmap_list) {
			mmap_list = new_block;
			new_block->prev = nullptr;
			new_block->next = nullptr;
		}
		else {
			mmap_list->next = new_block;
			new_block->prev = mmap_list;
			mmap_list = new_block;
			mmap_list->next = nullptr;
		}

		return (void *)((size_t)new_block + sizeof(MallocMetadata));
	}

	MallocMetadata * data = searchForAFreeBlock(size);
	MallocMetadata * block_to_allocate = splitBlock(data, size);

	if (block_to_allocate == nullptr) {
		return nullptr;
	}
	if (array_of_lists[data->order] == data) {
		array_of_lists[data->order] = data->next_free;
	}
	if (data->prev_free) {
		data->prev_free->next_free = data->next_free;
	}
	if (data->next_free) {
		data->next_free->prev_free = data->prev_free;
	}
	data->next_free = nullptr;
	data->prev_free = nullptr;
	data->is_free = false;
	return (void *)((size_t)block_to_allocate + sizeof(MallocMetadata));
}

void * scalloc(size_t num, size_t size) 
{
	void * ptr = smalloc(num * size);

	if (ptr == nullptr) {
		return nullptr;
	}
	memset(ptr, 0, num * size);
	return ptr;
}

void sfree(void * p) 
{
	if (p == nullptr) {
		return;
	}

	MallocMetadata * block_to_free = (MallocMetadata *)((size_t)p - sizeof(MallocMetadata));

	if (block_to_free->is_free) {
		return;
	}
	if (block_to_free->cookie != COOKIE_VALUE) {
		exit(0xdeadbeef);
	}
	if (block_to_free->size > LARGE_BLOCKS_THRESHOLD) {
		if (!mmap_list) {
			munmap((void *)block_to_free, block_to_free->size + sizeof(MallocMetadata));
			return;
		}
		if (block_to_free->prev == nullptr) {
			if (block_to_free->next == nullptr) {
				mmap_list = nullptr;
				block_to_free->next = nullptr;
				munmap((void *)block_to_free, block_to_free->size + sizeof(MallocMetadata));
				return;
			}
			else {
				block_to_free->next->prev = nullptr;
				block_to_free->next = nullptr;
				munmap((void *)block_to_free, block_to_free->size + sizeof(MallocMetadata));
				return;
			}
		}
		else if (block_to_free->next == nullptr) {
			mmap_list = block_to_free->prev;
			mmap_list->next = nullptr;
			munmap((void *)block_to_free, block_to_free->size + sizeof(MallocMetadata));
			return;
		}
		else {
			block_to_free->prev->next = block_to_free->next;
			block_to_free->next->prev = block_to_free->prev;
			block_to_free->next = nullptr;
			block_to_free->prev = nullptr;
			munmap((void *)block_to_free, block_to_free->size + sizeof(MallocMetadata));
			return;
		}
	}
	else {
		add_free(block_to_free);
		mergeBlocks(block_to_free);
	}
}

void * srealloc(void * oldp, size_t size) 
{
	if (size == 0 || size > MAX) {
		return nullptr;
	}
	if (oldp == nullptr) {
		return smalloc(size);
	}

	MallocMetadata * block_to_realloc = (MallocMetadata *)oldp - 1;

	if (block_to_realloc->size >= size) {
		return oldp;
	}
	void * newp = smalloc(size);
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
	MallocMetadata * current_block;

	for (int i = 0; i <= MAX_ORDER; i++) {
		current_block = array_of_lists[i];
		while (current_block != nullptr) {
			num_free_blocks++;
			current_block = current_block->next_free;
		}
	}
	return num_free_blocks;
}

size_t _num_free_bytes() 
{
	size_t num_free_bytes = 0;
	MallocMetadata * current_block;

	for (int i = 0; i <= MAX_ORDER; i++) {
		current_block = array_of_lists[i];
		while (current_block != nullptr) {
			num_free_bytes += current_block->size;
			current_block = current_block->next_free;
		}
	}
	return num_free_bytes;
}

size_t _num_allocated_blocks() 
{
	size_t num_allocated_blocks = 0;
	MallocMetadata * current_block = start;

	while (current_block != nullptr) {
		num_allocated_blocks++;
		current_block = current_block->next;
	}
	current_block = mmap_list;
	while (current_block != nullptr) {
		num_allocated_blocks++;
		current_block = current_block->prev;
	}
	return num_allocated_blocks;
}

size_t _num_allocated_bytes() 
{
	size_t num_allocated_bytes = 0;
	MallocMetadata * current_block = start;

	while (current_block != nullptr) {
		num_allocated_bytes += current_block->size;
		current_block = current_block->next;
	}
	current_block = mmap_list;
	while (current_block != nullptr) {
		num_allocated_bytes += current_block->size;
		current_block = current_block->prev;
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