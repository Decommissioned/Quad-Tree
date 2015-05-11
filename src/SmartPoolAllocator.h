#ifndef _SMART_POOL_ALLOCATOR_H
#define _SMART_POOL_ALLOCATOR_H

#include "config.h"
#include <memory>
#include <iostream>

#ifdef SMART_ALLOCATOR_DIAGNOSTICS
#define SMART_ALLOCATOR_ENABLE_PRINT 1
#else
#define SMART_ALLOCATOR_ENABLE_PRINT 0
#endif

namespace ORC_NAMESPACE
{
        /*
        Calculates the alignment requirement for the memory address
        */
        inline static size_t alignment_needed(void* address)
        {
                const size_t alignment = sizeof(void*);
                size_t required = alignment - (size_t) address % alignment;
                return required * (required != alignment);
        }

        inline static void* ptr_add(void* address, size_t offset)
        {
                return static_cast<char*>(address) +offset;
        }

        struct Bookkeeper
        {
                size_t size;
                Bookkeeper* next;
        };
        struct MemoryPool
        {
                void* end;
                void* current;
                Bookkeeper* bk_first;
        };

        /*
        Description: creates a block of memory that can be used by allocators
        Remark: std::free must be called on it after using it, otherwise the memory will leak
        */
        MemoryPool* MakeMemoryPool(size_t size)
        {
                size_t actual = size + sizeof(MemoryPool);
                MemoryPool* pool = (MemoryPool*) std::malloc(actual);
                pool->end = (char*) pool + actual;
                pool->current = (char*) pool + sizeof(MemoryPool);
                pool->bk_first = nullptr;

                return pool;
        }

        /*
        This allocator is blazingly fast until it runs out of pool memory, after that it will be as fast as the standard allocator plus some overhead.
        Freeing memory will speed it back up a bit, however if the sizes of the allocations vary a lot memory fragmentation will become a serious issue.
        REMARK: When using this allocator with a container (like STL) you will have to ensure that the pool's memory will be available until all containers using it are DESTROYED.
        REMARK: For STL containers, a constructed instance of the allocator should be passed a parameter to the container's constructor, otherwise states will not work
        REMARK: If no memory pool is provided, the standard allocator will be used instead
        TODO: Concatenate adjacent memory in bookkeeping list, haven't thought of a good way of doing this yet
        */
        template <typename T>
        struct SmartPoolAllocator
        {
                std::allocator<T> fallback;
                MemoryPool* pool;

                // Requirements for standard conformity, also allows this to work with STL containers
                using value_type = T;
                using propagate_on_container_move_assignment = std::true_type;

                SmartPoolAllocator() throw() = delete;
                SmartPoolAllocator(MemoryPool* pool) throw() : pool(pool)
                {}
                template <typename U> SmartPoolAllocator(const SmartPoolAllocator<U>& o) throw() : pool(o.pool)
                {}
                template <typename U> SmartPoolAllocator(const SmartPoolAllocator<U>&&) = delete;
                template <typename U> const SmartPoolAllocator& operator=(const SmartPoolAllocator<U>&) = delete;
                template <typename U> const SmartPoolAllocator& operator=(const SmartPoolAllocator<U>&&) = delete;
                ~SmartPoolAllocator() throw()
                {}

                T* allocate(size_t size, void* = 0)
                {
                        // If the requested size is too small we have to live with either not being able to use bookkeeping or over allocate
                        size_t requested_bytes = size * sizeof(T);
                        size_t actual_bytes = requested_bytes > sizeof(Bookkeeper) ? requested_bytes : sizeof(Bookkeeper);

                        // First of all, alignment requirements are checked, we can ignore the original pointer since std::free wont be needing it
                        size_t align_size = alignment_needed(pool->current);
                        T* address = static_cast<T*>(ptr_add(pool->current, align_size));

                        // Next, would offsetting the pointer by the required amount cause buffer overflow?
                        void* next = ptr_add(address, actual_bytes);
                        if (next >= pool->end)
                        {
                                // Pool is full, search for available rooms
                                Bookkeeper* prev = nullptr;
                                Bookkeeper* node = pool->bk_first;
                                while (node)
                                {
                                        if (node->size >= actual_bytes) // Does it fit? Alignment checks not needed here, it's already aligned
                                        {
                                                if (SMART_ALLOCATOR_ENABLE_PRINT)
                                                        std::cout << "Bookkeeping: claimed " << node->size << " bytes at [0x" << node << ']' << std::endl;

                                                // Make the previous node point to the next node
                                                if (prev != nullptr) prev->next = node->next;
                                                // First node in the list, make the head point to the next element
                                                else pool->bk_first = node->next;

                                                return reinterpret_cast<T*>(node);
                                        }

                                        prev = node;
                                        node = node->next;
                                }

                                // No room to perform allocation, use fallback allocator
                                if (SMART_ALLOCATOR_ENABLE_PRINT)
                                        std::cout << "Allocation: no memory, using fallback allocator" << std::endl;

                                return fallback.allocate(size);
                        }

                        pool->current = next;

                        if (SMART_ALLOCATOR_ENABLE_PRINT)
                                std::cout << "Allocation: " << actual_bytes << " bytes requested at [0x" << pool->current << ']' << std::endl;

                        return address;
                }

                void deallocate(T* address, size_t n)
                {
                        // If the address is not inside the block used by the pool, assume it's from the fallback allocatior
                        if (reinterpret_cast<void*>(address) < reinterpret_cast<void*>(pool) ||
                            reinterpret_cast<void*>(address) > pool->end)
                        {
                                if (SMART_ALLOCATOR_ENABLE_PRINT)
                                        std::cout << "Deallocate: memory from fallback allocator" << std::endl;

                                fallback.deallocate(address, n);
                                return;
                        }

                        // Otherwise, add the returned memory to bookkeeping
                        Bookkeeper* node = reinterpret_cast<Bookkeeper*>(address);
                        node->next = pool->bk_first;
                        node->size = n * sizeof(T);
                        pool->bk_first = node;

                        if (SMART_ALLOCATOR_ENABLE_PRINT)
                                std::cout << "Bookkeeping: stored " << node->size << " bytes at [0x" << address << ']' << std::endl;
                }

        };
        template<typename T>
        bool operator==(const SmartPoolAllocator<T>&, const SmartPoolAllocator<T>&)
        {
                return true;
        }
        template<typename T>
        bool operator!=(const SmartPoolAllocator<T>&, const SmartPoolAllocator<T>&)
        {
                return false;
        }

};

#endif // _SMART_POOL_ALLOCATOR
