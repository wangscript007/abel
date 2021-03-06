//
// Created by liyinbin on 2020/3/30.
//


#include <stdlib.h>
#include <algorithm>
#include <abel/memory/arena.h>

namespace abel {


    arena_options::arena_options()
            : initial_block_size(64)
            , max_block_size(8192)
    {}

    arena::arena(const arena_options& options)
            : _cur_block(nullptr)
            , _isolated_blocks(nullptr)
            , _block_size(options.initial_block_size)
            , _options(options) {
    }

    arena::~arena() {
        while (_cur_block != nullptr) {
            block* const saved_next = _cur_block->next;
            free(_cur_block);
            _cur_block = saved_next;
        }
        while (_isolated_blocks != nullptr) {
            block* const saved_next = _isolated_blocks->next;
            free(_isolated_blocks);
            _isolated_blocks = saved_next;
        }
    }

    void arena::swap(arena& other) {
        std::swap(_cur_block, other._cur_block);
        std::swap(_isolated_blocks, other._isolated_blocks);
        std::swap(_block_size, other._block_size);
        const arena_options tmp = _options;
        _options = other._options;
        other._options = tmp;
    }

    void arena::clear() {
        // TODO(gejun): Reuse memory
        arena a;
        swap(a);
    }

    void* arena::allocate_new_block(size_t n) {
        block* b = (block*)malloc(offsetof(block, data) + n);
        b->next = _isolated_blocks;
        b->alloc_size = n;
        b->size = n;
        _isolated_blocks = b;
        return b->data;
    }

    void* arena::allocate_in_other_blocks(size_t n) {
        if (n > _block_size / 4) { // put outlier on separate blocks.
            return allocate_new_block(n);
        }
        // Waste the left space. At most 1/4 of allocated spaces are wasted.

        // Grow the block size gradually.
        if (_cur_block != nullptr) {
            _block_size = std::min(2 * _block_size, _options.max_block_size);
        }
        size_t new_size = _block_size;
        if (new_size < n) {
            new_size = n;
        }
        block* b = (block*)malloc(offsetof(block, data) + new_size);
        if (nullptr == b) {
            return nullptr;
        }
        b->next = nullptr;
        b->alloc_size = n;
        b->size = new_size;
        if (_cur_block) {
            _cur_block->next = _isolated_blocks;
            _isolated_blocks = _cur_block;
        }
        _cur_block = b;
        return b->data;
    }

}  // namespace abel

