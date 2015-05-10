#ifndef _SMART_POOL_ALLOCATOR_H
#define _SMART_POOL_ALLOCATOR_H

#include "config.h"
#include <memory>
#include <iostream>

#define SMARTALLOCATOR_DIAGNOSTICS 0

namespace ORC_NAMESPACE
{

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

        // I totally leak memory if you don't free me after you're done
        MemoryPool* MakeMemoryPool(size_t size)
        {
                size_t actual = size + sizeof(MemoryPool);
                MemoryPool* pool = (MemoryPool*) std::malloc(actual);
                pool->end = (char*) pool + actual;
                pool->current = (char*) pool + sizeof(MemoryPool);
                pool->bk_first = nullptr;

                if (SMARTALLOCATOR_DIAGNOSTICS)
                {
                        std::cout << "pool: [0x" << pool << "]\n"
                                << "pool->end: [0x" << pool->end << "]\n"
                                << "pool->current: [0x" << pool->current << ']' << std::endl;
                }

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

                // This is a relatively complex implementation, but the performance should be blazingly fast
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
                                // Even if no more memory is available in the pool, it's possible that there is still enough memory for an allocation in the bookkeeping list
                                Bookkeeper* prev = nullptr;
                                Bookkeeper* node = pool->bk_first;
                                while (node)
                                {
                                        if (node->size >= actual_bytes) // Does it fit?
                                        {
                                                if (SMARTALLOCATOR_DIAGNOSTICS)
                                                        std::cout << "Found " << node->size << " bytes in the bookkeeping tree at [0x" << node << ']' << std::endl;

                                                if (prev != nullptr) // Re-link the list if necessary
                                                {
                                                        prev->next = node->next;
                                                }
                                                else // Make the next element the beginning of the list
                                                {
                                                        pool->bk_first = node->next;
                                                }

                                                return reinterpret_cast<T*>(node);
                                        }

                                        prev = node;
                                        node = node->next;
                                }

                                if (SMARTALLOCATOR_DIAGNOSTICS)
                                        std::cout << "No memory available, using fallback allocator" << std::endl;

                                // If so, a fallback allocator is used, which is slow but will prevent the program from crashing
                                return fallback.allocate(size);
                        }

                        pool->current = next;

                        if (SMARTALLOCATOR_DIAGNOSTICS)
                                std::cout << "Allocation of " << actual_bytes << " bytes requested at + " << align_size << " [0x" << pool->current << "], " << reinterpret_cast<char*>(pool->end) - reinterpret_cast<char*>(pool->current) << " bytes remaining" << std::endl;

                        return address;
                }

                void deallocate(T* ptr, size_t n)
                {
                        // If the memory being returned is outside the boundaries of the pool, assume it's from fallback allocation
                        if (reinterpret_cast<void*>(ptr) < reinterpret_cast<void*>(pool) ||
                            reinterpret_cast<void*>(ptr) > pool->end)
                        {
                                if (SMARTALLOCATOR_DIAGNOSTICS)
                                        std::cout << "Calling deallocate from fallback allocator" << std::endl;

                                fallback.deallocate(ptr, n);
                                return;
                        }

                        // Add the memory to bookkeeping list
                        Bookkeeper* node = reinterpret_cast<Bookkeeper*>(ptr);
                        node->next = pool->bk_first;
                        node->size = n * sizeof(T);
                        pool->bk_first = node;

                        if (SMARTALLOCATOR_DIAGNOSTICS)
                                std::cout << "Deallocation requested, added to bookkeeping " << node->size << " bytes at [0x" << ptr << ']' << std::endl;
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
