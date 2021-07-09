#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>

typedef struct header {
    size_t size;
    struct header* next;
    struct header* prev;
    int is_free;
    int marked;
} header_t;

void* malloc_t(size_t size);
void free_t(void* p);
void free_block(header_t* p);
void mark(struct header* object);

#define HEADER_SIZE sizeof(header_t);
#define STACK_MAX_SIZE 256

typedef struct virtule_memory {
    header_t* stack[STACK_MAX_SIZE];
    header_t* first_obj;
    int stack_size;
    int objects_amount;
} virtule_memory_t;

virtule_memory_t vm;
header_t* first_block = NULL;

virtule_memory_t newVM() {
    virtule_memory_t vm;
    vm.stack_size = 0;
    vm.first_obj = NULL;
    vm.objects_amount = 0;
    return vm;
}

void* pop() {
    vm.stack_size--;
    return vm.stack[vm.stack_size] + HEADER_SIZE;
}

void markAll() {
    for (int i=0; i<vm.stack_size; i++) {
        mark(vm.stack[i]);
        printf("%p\n", vm.stack[i]);
    }
}

void mark(header_t* object) {
    if (object->marked) return;
    object->marked = 1;
}

void sweep() {
    header_t* object = vm.first_obj;
    while(object) {
        if (object->marked) {
            object->marked = 0;
            object = object->prev;
        }
        else {
            header_t* unreached = object;
            object = object->prev; 

            free_block(unreached);
            vm.objects_amount--;
        }
    }
}

void gc() {
    int before = vm.objects_amount;
    markAll();
    sweep(); 
    int after = vm.objects_amount;

    printf("Objects collected: [%d]. Before: [%d]. After: [%d]", before-after, before, after);
}

void LOG() {
    header_t* current = first_block; 
    printf("\n---------LOG----------\n");
    while (current) {
        printf("Address: [%p]. Size: [%ld]. Free: [%d]\n", current, current->size, current->is_free);
        current = current->next;
    }
    printf("----------------------\n\n");
}

void split_block(header_t* block, size_t size) {
        header_t* new_block = block + size;
        new_block->size = block->size - size;
        new_block->next = block->next;
        new_block->is_free = 1;
        
        block->size = size;
        block->next = new_block;
}

header_t* find_free_block(header_t* first_block, size_t size) {
    header_t* free_block = NULL; // the smallest block which can contain [size]
    header_t* current = first_block;
    while (current) {
        if (current->is_free) {
            free_block = current;
            if (current->size >= 2*size) {
                split_block(current, size);
            }
            return free_block;
        }
        current = current->next;
    }
    return free_block;
}

header_t* request_memory(header_t* last, size_t size) {
    header_t* header;
    header = sbrk(0);
    void *request = sbrk(size);
    assert((void*)header == request);
    if (header == (void*)-1) {
        return NULL;
    }
    
    if (last) {
        last->next = header;
        header->prev = last;
    }
    else {  
        first_block = header;
    }
    
    header->size = size;
    header->next = NULL;
    header->is_free = 0;
    
    return header;
}  

void* malloc_t(size_t size) {    
    header_t* block;
    static header_t* last;
    
    if (size <= 0) {
        return NULL;
    }

    size += HEADER_SIZE; 
    
    if (!first_block) {
        block = request_memory(NULL, size);
        if (!block) {
            return NULL;
        }
    }
    else {
        block = find_free_block(first_block, size);
        if (!block) {
            block = request_memory(last, size);
            if (!block) {
                return NULL;
            }
        }
        else {
            block->is_free = 0;
        }
    }

    vm.stack[vm.stack_size++] = block;
    vm.first_obj = block;
    vm.objects_amount++;

    last = block;
    return (block+1);
}

void free_t(void* p) {
    if (p == NULL) {
        return;
    }
    header_t* h = p - HEADER_SIZE;
    h->is_free = 1;
    
    printf("In free_t(): [%p]. is_free: [%d]\n", h, h->is_free);
    
    header_t* cur = first_block;
    while(cur) {
        if (cur->next) {
            if (cur->is_free && cur->next->is_free) {
                cur->size += cur->next->size;
                cur->next = cur->next->next;
            }
        }
        cur = cur->next;
    }
}

void free_block(header_t* p) {
    if (p == NULL) {
        return;
    }
    p->is_free = 1;
    
    printf("In free_t(): [%p]. is_free: [%d]\n", p, p->is_free);
    
    header_t* cur = first_block;
    while(cur) {
        if (cur->next) {
            if (cur->is_free && cur->next->is_free) {
                cur->size += cur->next->size;
                cur->next = cur->next->next;
            }
        }
        cur = cur->next;
    }
}

void test() {
    int* a = malloc_t(sizeof(int));
    int* b = malloc_t(sizeof(int));
    int* c = malloc_t(sizeof(int));
    int* d = malloc_t(sizeof(int));
    int* e = malloc_t(sizeof(int));
    int* f = malloc_t(sizeof(int));
    int* g = malloc_t(sizeof(int));
    int* h = malloc_t(sizeof(int));
    int* j = malloc_t(sizeof(int));
    int* k = malloc_t(sizeof(int));
    printf("%d\n", vm.stack_size);
    header_t* first = pop();
    header_t* second = pop();
    header_t* third = pop();
    printf("Poped: [%p], [%p], [%p]\n", first, second, third);
    printf("%d\n", vm.stack_size);
    gc();
    LOG();
}

int main(int argc, char** argv) {
    vm = newVM();
    test();
    return 0;
}