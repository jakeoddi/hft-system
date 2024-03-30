#pragma once

#include <vector>
#include <cstdint>

#include "macros.hpp"

namespace common {
    template <typename T>
    class MemPool {
    public:
        explicit MemPool(int capacity) : 
            capacity_(capacity), 
            store_(capacity),
            size_(0), 
            next_free_idx_(0) {
                // check that the T object of the first ObjectBlock
                // has the same address as the first ObjectBlock
                // in the vector, i.e. check that the T objects are 
                // aligned first in each ObjectBlock for optimal padding
                // TODO: can use static_assert?
                ASSERT(    
                    reinterpret_cast<const ObjectBlock*>(&store_[0].obj_) == &(store_[0]),
                    "T object should be the first member of ObjectBlock"
                );
            }
        MemPool() = delete;
        MemPool(const MemPool&) = delete;
        MemPool(const MemPool&&) = delete;
        MemPool& operator=(const MemPool&) = delete;
        MemPool& operator=(const MemPool&&) = delete;

        /*
         * User should not initialize the target object. Instead, pass 
         * args directly to `allocate`, which will take care of object
         * construction.
        */
        template <typename... A>
        T* allocate(A&&... args) {
            // start by finding a free index
            update_next_free_idx();
            // TODO: check if there's available space. if yes, use 
            // next_free_idx_ to allocate. if not, use least recently used?
            ASSERT(size_ < capacity_, "MemPool is full");
            auto new_block = &store_[next_free_idx_];
            ASSERT(
                new_block->is_free_, 
                "Expected free ObjectBlock at index: " + std::to_string(next_free_idx_)
            );
            // placement new: constructs the new object in MemPool instead of 
            // default behavior of allocating new heap memory
            T* new_obj = new (&new_block->obj_) T(std::forward<A>(args)...);
            new_block->is_free_ = false;
            ++size_;
            std::cout << "Successfully allocated at index " << next_free_idx_ << 
            ". New size = " << size_ << std::endl;
            return new_obj;
        }

        void deallocate(const T* obj) {
            // compute the index of the ObjectBlock of `obj` in store_
            const auto dealloc_idx = reinterpret_cast<const ObjectBlock *>(obj) - &store_[0];
            ASSERT(
                dealloc_idx >= 0 && static_cast<size_t>(dealloc_idx) < capacity_,
                "Element being deallocated does not belong to this Memory pool."
            );
            ASSERT(
                !store_[dealloc_idx].is_free_, 
                "Expected in-use ObjectBlock at index: " + std::to_string(dealloc_idx)
            );
            store_[dealloc_idx].is_free_ = true;
            --size_;
            std::cout << "Successfully dealloacted at index " << dealloc_idx << 
            ". New size = " << size_ << std::endl;
        }
    
    private:
        struct ObjectBlock {
            // order from largest to smallest to minimize padding
            T obj_;
            bool is_free_ = true;
        };

        std::vector<ObjectBlock> store_;
        size_t size_;
        size_t capacity_;
        size_t next_free_idx_;

        void update_next_free_idx() {
            for (size_t i = 0; i < capacity_; ++i) {
                size_t j = (next_free_idx_+i) % capacity_;
                if (store_[j].is_free_) {
                    next_free_idx_ = j;
                    return;
                }
            }
            FATAL("Unable to find a free ObjectBlock in MemPool");
        }
    };
}