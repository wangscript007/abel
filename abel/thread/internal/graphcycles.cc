//

// graph_cycles provides incremental cycle detection on a dynamic
// graph using the following algorithm:
//
// A dynamic topological sort algorithm for directed acyclic graphs
// David J. Pearce, Paul H. J. Kelly
// Journal of Experimental Algorithmics (JEA) JEA Homepage archive
// Volume 11, 2006, Article No. 1.7
//
// Brief summary of the algorithm:
//
// (1) Maintain a rank for each node that is consistent
//     with the topological sort of the graph. I.e., path from x to y
//     implies rank[x] < rank[y].
// (2) When a new edge (x->y) is inserted, do nothing if rank[x] < rank[y].
// (3) Otherwise: adjust ranks in the neighborhood of x and y.

#include <abel/base/profile.h>
// This file is a no-op if the required low_level_alloc support is missing.
#include <abel/memory/internal/low_level_alloc.h>

#ifndef ABEL_LOW_LEVEL_ALLOC_MISSING

#include <abel/thread/internal/graphcycles.h>

#include <algorithm>
#include <array>
#include <abel/memory/hide_ptr.h>
#include <abel/log/logging.h>
#include <abel/thread/internal/spinlock.h>

// Do not use STL.   This module does not use standard memory allocation.

namespace abel {

    namespace thread_internal {

        namespace {

// Avoid low_level_alloc's default arena since it calls malloc hooks in
// which people are doing things like acquiring Mutexes.
            static abel::thread_internal::spin_lock arena_mu(
                    abel::base_internal::kLinkerInitialized);
            static memory_internal::low_level_alloc::arena *arena;

            static void InitArenaIfNecessary() {
                arena_mu.lock();
                if (arena == nullptr) {
                    arena = memory_internal::low_level_alloc::new_arena(0);
                }
                arena_mu.unlock();
            }

// Number of inlined elements in Vec.  Hash table implementation
// relies on this being a power of two.
            static const uint32_t kInline = 8;

// A simple low_level_alloc based resizable vector with inlined storage
// for a few elements.  T must be a plain type since constructor
// and destructor are not run on elements of type T managed by Vec.
            template<typename T>
            class Vec {
            public:
                Vec() { Init(); }

                ~Vec() { Discard(); }

                void clear() {
                    Discard();
                    Init();
                }

                bool empty() const { return size_ == 0; }

                uint32_t size() const { return size_; }

                T *begin() { return ptr_; }

                T *end() { return ptr_ + size_; }

                const T &operator[](uint32_t i) const { return ptr_[i]; }

                T &operator[](uint32_t i) { return ptr_[i]; }

                const T &back() const { return ptr_[size_ - 1]; }

                void pop_back() { size_--; }

                void push_back(const T &v) {
                    if (size_ == capacity_) Grow(size_ + 1);
                    ptr_[size_] = v;
                    size_++;
                }

                void resize(uint32_t n) {
                    if (n > capacity_) Grow(n);
                    size_ = n;
                }

                void fill(const T &val) {
                    for (uint32_t i = 0; i < size(); i++) {
                        ptr_[i] = val;
                    }
                }

                // Guarantees src is empty at end.
                // Provided for the hash table resizing code below.
                void MoveFrom(Vec<T> *src) {
                    if (src->ptr_ == src->space_) {
                        // Need to actually copy
                        resize(src->size_);
                        std::copy(src->ptr_, src->ptr_ + src->size_, ptr_);
                        src->size_ = 0;
                    } else {
                        Discard();
                        ptr_ = src->ptr_;
                        size_ = src->size_;
                        capacity_ = src->capacity_;
                        src->Init();
                    }
                }

            private:
                T *ptr_;
                T space_[kInline];
                uint32_t size_;
                uint32_t capacity_;

                void Init() {
                    ptr_ = space_;
                    size_ = 0;
                    capacity_ = kInline;
                }

                void Discard() {
                    if (ptr_ != space_) memory_internal::low_level_alloc::free(ptr_);
                }

                void Grow(uint32_t n) {
                    while (capacity_ < n) {
                        capacity_ *= 2;
                    }
                    size_t request = static_cast<size_t>(capacity_) * sizeof(T);
                    T *copy = static_cast<T *>(
                            memory_internal::low_level_alloc::alloc_with_arena(request, arena));
                    std::copy(ptr_, ptr_ + size_, copy);
                    Discard();
                    ptr_ = copy;
                }

                Vec(const Vec &) = delete;

                Vec &operator=(const Vec &) = delete;
            };

// A hash set of non-negative int32_t that uses Vec for its underlying storage.
            class NodeSet {
            public:
                NodeSet() { Init(); }

                void clear() { Init(); }

                bool contains(int32_t v) const { return table_[FindIndex(v)] == v; }

                bool insert(int32_t v) {
                    uint32_t i = FindIndex(v);
                    if (table_[i] == v) {
                        return false;
                    }
                    if (table_[i] == kEmpty) {
                        // Only inserting over an empty cell increases the number of occupied
                        // slots.
                        occupied_++;
                    }
                    table_[i] = v;
                    // Double when 75% full.
                    if (occupied_ >= table_.size() - table_.size() / 4) Grow();
                    return true;
                }

                void erase(uint32_t v) {
                    uint32_t i = FindIndex(v);
                    if (static_cast<uint32_t>(table_[i]) == v) {
                        table_[i] = kDel;
                    }
                }

                // Iteration: is done via HASH_FOR_EACH
                // Example:
                //    HASH_FOR_EACH(elem, node->out) { ... }
#define HASH_FOR_EACH(elem, eset) \
  for (int32_t elem, _cursor = 0; (eset).Next(&_cursor, &elem); )

                bool Next(int32_t *cursor, int32_t *elem) {
                    while (static_cast<uint32_t>(*cursor) < table_.size()) {
                        int32_t v = table_[*cursor];
                        (*cursor)++;
                        if (v >= 0) {
                            *elem = v;
                            return true;
                        }
                    }
                    return false;
                }

            private:
                enum : int32_t {
                    kEmpty = -1, kDel = -2
                };
                Vec<int32_t> table_;
                uint32_t occupied_;     // Count of non-empty slots (includes deleted slots)

                static uint32_t Hash(uint32_t a) { return a * 41; }

                // Return index for storing v.  May return an empty index or deleted index
                int FindIndex(int32_t v) const {
                    // Search starting at hash index.
                    const uint32_t mask = table_.size() - 1;
                    uint32_t i = Hash(v) & mask;
                    int deleted_index = -1;  // If >= 0, index of first deleted element we see
                    while (true) {
                        int32_t e = table_[i];
                        if (v == e) {
                            return i;
                        } else if (e == kEmpty) {
                            // Return any previously encountered deleted slot.
                            return (deleted_index >= 0) ? deleted_index : i;
                        } else if (e == kDel && deleted_index < 0) {
                            // Keep searching since v might be present later.
                            deleted_index = i;
                        }
                        i = (i + 1) & mask;  // Linear probing; quadratic is slightly slower.
                    }
                }

                void Init() {
                    table_.clear();
                    table_.resize(kInline);
                    table_.fill(kEmpty);
                    occupied_ = 0;
                }

                void Grow() {
                    Vec<int32_t> copy;
                    copy.MoveFrom(&table_);
                    occupied_ = 0;
                    table_.resize(copy.size() * 2);
                    table_.fill(kEmpty);

                    for (const auto &e : copy) {
                        if (e >= 0) insert(e);
                    }
                }

                NodeSet(const NodeSet &) = delete;

                NodeSet &operator=(const NodeSet &) = delete;
            };

// We encode a node index and a node version in graph_id.  The version
// number is incremented when the graph_id is freed which automatically
// invalidates all copies of the graph_id.

            ABEL_FORCE_INLINE graph_id MakeId(int32_t index, uint32_t version) {
                graph_id g;
                g.handle =
                        (static_cast<uint64_t>(version) << 32) | static_cast<uint32_t>(index);
                return g;
            }

            ABEL_FORCE_INLINE int32_t NodeIndex(graph_id id) {
                return static_cast<uint32_t>(id.handle & 0xfffffffful);
            }

            ABEL_FORCE_INLINE uint32_t NodeVersion(graph_id id) {
                return static_cast<uint32_t>(id.handle >> 32);
            }

            struct Node {
                int32_t rank;               // rank number assigned by Pearce-Kelly algorithm
                uint32_t version;           // Current version number
                int32_t next_hash;          // Next entry in hash table
                bool visited;               // Temporary marker used by depth-first-search
                uintptr_t masked_ptr;       // User-supplied pointer
                NodeSet in;                 // List of immediate predecessor nodes in graph
                NodeSet out;                // List of immediate successor nodes in graph
                int priority;               // Priority of recorded stack trace.
                int nstack;                 // Depth of recorded stack trace.
                void *stack[40];            // stack[0,nstack-1] holds stack trace for node.
            };

// Hash table for pointer to node index lookups.
            class PointerMap {
            public:
                explicit PointerMap(const Vec<Node *> *nodes) : nodes_(nodes) {
                    table_.fill(-1);
                }

                int32_t Find(void *ptr) {
                    auto masked = hide_ptr(ptr);
                    for (int32_t i = table_[Hash(ptr)]; i != -1;) {
                        Node *n = (*nodes_)[i];
                        if (n->masked_ptr == masked) return i;
                        i = n->next_hash;
                    }
                    return -1;
                }

                void Add(void *ptr, int32_t i) {
                    int32_t *head = &table_[Hash(ptr)];
                    (*nodes_)[i]->next_hash = *head;
                    *head = i;
                }

                int32_t Remove(void *ptr) {
                    // Advance through linked list while keeping track of the
                    // predecessor slot that points to the current entry.
                    auto masked = hide_ptr(ptr);
                    for (int32_t *slot = &table_[Hash(ptr)]; *slot != -1;) {
                        int32_t index = *slot;
                        Node *n = (*nodes_)[index];
                        if (n->masked_ptr == masked) {
                            *slot = n->next_hash;  // Remove n from linked list
                            n->next_hash = -1;
                            return index;
                        }
                        slot = &n->next_hash;
                    }
                    return -1;
                }

            private:
                // Number of buckets in hash table for pointer lookups.
                static constexpr uint32_t kHashTableSize = 8171;  // should be prime

                const Vec<Node *> *nodes_;
                std::array<int32_t, kHashTableSize> table_;

                static uint32_t Hash(void *ptr) {
                    return reinterpret_cast<uintptr_t>(ptr) % kHashTableSize;
                }
            };

        }  // namespace

        struct graph_cycles::Rep {
            Vec<Node *> nodes_;
            Vec<int32_t> free_nodes_;  // Indices for unused entries in nodes_
            PointerMap ptrmap_;

            // Temporary state.
            Vec<int32_t> deltaf_;  // Results of forward DFS
            Vec<int32_t> deltab_;  // Results of backward DFS
            Vec<int32_t> list_;    // All nodes to reprocess
            Vec<int32_t> merged_;  // Rank values to assign to list_ entries
            Vec<int32_t> stack_;   // Emulates recursion stack for depth-first searches

            Rep() : ptrmap_(&nodes_) {}
        };

        static Node *FindNode(graph_cycles::Rep *rep, graph_id id) {
            Node *n = rep->nodes_[NodeIndex(id)];
            return (n->version == NodeVersion(id)) ? n : nullptr;
        }

        graph_cycles::graph_cycles() {
            InitArenaIfNecessary();
            rep_ = new(memory_internal::low_level_alloc::alloc_with_arena(sizeof(Rep), arena))
                    Rep;
        }

        graph_cycles::~graph_cycles() {
            for (auto *node : rep_->nodes_) {
                node->Node::~Node();
                memory_internal::low_level_alloc::free(node);
            }
            rep_->Rep::~Rep();
            memory_internal::low_level_alloc::free(rep_);
        }

        bool graph_cycles::check_invariants() const {
            Rep *r = rep_;
            NodeSet ranks;  // Set of ranks seen so far.
            for (uint32_t x = 0; x < r->nodes_.size(); x++) {
                Node *nx = r->nodes_[x];
                void *ptr = unhide_ptr<void>(nx->masked_ptr);
                if (ptr != nullptr && static_cast<uint32_t>(r->ptrmap_.Find(ptr)) != x) {
                    DLOG_CRITICAL("Did not find live node in hash table {} {}", x, ptr);
                }
                if (nx->visited) {
                    DLOG_CRITICAL("Did not clear visited marker on node {}", x);
                }
                if (!ranks.insert(nx->rank)) {
                    DLOG_CRITICAL("Duplicate occurrence of rank %d", nx->rank);
                }
                HASH_FOR_EACH(y, nx->out) {
                    Node *ny = r->nodes_[y];
                    if (nx->rank >= ny->rank) {
                        DLOG_CRITICAL("Edge {}->{} has bad rank assignment {}->{}", x, y,
                                     nx->rank, ny->rank);
                    }
                }
            }
            return true;
        }

        graph_id graph_cycles::get_id(void *ptr) {
            int32_t i = rep_->ptrmap_.Find(ptr);
            if (i != -1) {
                return MakeId(i, rep_->nodes_[i]->version);
            } else if (rep_->free_nodes_.empty()) {
                Node *n =
                        new(memory_internal::low_level_alloc::alloc_with_arena(sizeof(Node), arena))
                                Node;
                n->version = 1;  // Avoid 0 since it is used by invalid_graphId()
                n->visited = false;
                n->rank = rep_->nodes_.size();
                n->masked_ptr = hide_ptr(ptr);
                n->nstack = 0;
                n->priority = 0;
                rep_->nodes_.push_back(n);
                rep_->ptrmap_.Add(ptr, n->rank);
                return MakeId(n->rank, n->version);
            } else {
                // Preserve preceding rank since the set of ranks in use must be
                // a permutation of [0,rep_->nodes_.size()-1].
                int32_t r = rep_->free_nodes_.back();
                rep_->free_nodes_.pop_back();
                Node *n = rep_->nodes_[r];
                n->masked_ptr = hide_ptr(ptr);
                n->nstack = 0;
                n->priority = 0;
                rep_->ptrmap_.Add(ptr, r);
                return MakeId(r, n->version);
            }
        }

        void graph_cycles::remove_node(void *ptr) {
            int32_t i = rep_->ptrmap_.Remove(ptr);
            if (i == -1) {
                return;
            }
            Node *x = rep_->nodes_[i];
            HASH_FOR_EACH(y, x->out) {
                rep_->nodes_[y]->in.erase(i);
            }
            HASH_FOR_EACH(y, x->in) {
                rep_->nodes_[y]->out.erase(i);
            }
            x->in.clear();
            x->out.clear();
            x->masked_ptr = hide_ptr<void>(nullptr);
            if (x->version == std::numeric_limits<uint32_t>::max()) {
                // Cannot use x any more
            } else {
                x->version++;  // Invalidates all copies of node.
                rep_->free_nodes_.push_back(i);
            }
        }

        void *graph_cycles::ptr(graph_id id) {
            Node *n = FindNode(rep_, id);
            return n == nullptr ? nullptr
                                : unhide_ptr<void>(n->masked_ptr);
        }

        bool graph_cycles::has_node(graph_id node) {
            return FindNode(rep_, node) != nullptr;
        }

        bool graph_cycles::has_edge(graph_id x, graph_id y) const {
            Node *xn = FindNode(rep_, x);
            return xn && FindNode(rep_, y) && xn->out.contains(NodeIndex(y));
        }

        void graph_cycles::remove_edge(graph_id x, graph_id y) {
            Node *xn = FindNode(rep_, x);
            Node *yn = FindNode(rep_, y);
            if (xn && yn) {
                xn->out.erase(NodeIndex(y));
                yn->in.erase(NodeIndex(x));
                // No need to update the rank assignment since a previous valid
                // rank assignment remains valid after an edge deletion.
            }
        }

        static bool ForwardDFS(graph_cycles::Rep *r, int32_t n, int32_t upper_bound);

        static void BackwardDFS(graph_cycles::Rep *r, int32_t n, int32_t lower_bound);

        static void Reorder(graph_cycles::Rep *r);

        static void Sort(const Vec<Node *> &, Vec<int32_t> *delta);

        static void MoveToList(
                graph_cycles::Rep *r, Vec<int32_t> *src, Vec<int32_t> *dst);

        bool graph_cycles::insert_edge(graph_id idx, graph_id idy) {
            Rep *r = rep_;
            const int32_t x = NodeIndex(idx);
            const int32_t y = NodeIndex(idy);
            Node *nx = FindNode(r, idx);
            Node *ny = FindNode(r, idy);
            if (nx == nullptr || ny == nullptr) return true;  // Expired ids

            if (nx == ny) return false;  // Self edge
            if (!nx->out.insert(y)) {
                // Edge already exists.
                return true;
            }

            ny->in.insert(x);

            if (nx->rank <= ny->rank) {
                // New edge is consistent with existing rank assignment.
                return true;
            }

            // Current rank assignments are incompatible with the new edge.  Recompute.
            // We only need to consider nodes that fall in the range [ny->rank,nx->rank].
            if (!ForwardDFS(r, y, nx->rank)) {
                // Found a cycle.  Undo the insertion and tell caller.
                nx->out.erase(y);
                ny->in.erase(x);
                // Since we do not call Reorder() on this path, clear any visited
                // markers left by ForwardDFS.
                for (const auto &d : r->deltaf_) {
                    r->nodes_[d]->visited = false;
                }
                return false;
            }
            BackwardDFS(r, x, ny->rank);
            Reorder(r);
            return true;
        }

        static bool ForwardDFS(graph_cycles::Rep *r, int32_t n, int32_t upper_bound) {
            // Avoid recursion since stack space might be limited.
            // We instead keep a stack of nodes to visit.
            r->deltaf_.clear();
            r->stack_.clear();
            r->stack_.push_back(n);
            while (!r->stack_.empty()) {
                n = r->stack_.back();
                r->stack_.pop_back();
                Node *nn = r->nodes_[n];
                if (nn->visited) continue;

                nn->visited = true;
                r->deltaf_.push_back(n);

                HASH_FOR_EACH(w, nn->out) {
                    Node *nw = r->nodes_[w];
                    if (nw->rank == upper_bound) {
                        return false;  // Cycle
                    }
                    if (!nw->visited && nw->rank < upper_bound) {
                        r->stack_.push_back(w);
                    }
                }
            }
            return true;
        }

        static void BackwardDFS(graph_cycles::Rep *r, int32_t n, int32_t lower_bound) {
            r->deltab_.clear();
            r->stack_.clear();
            r->stack_.push_back(n);
            while (!r->stack_.empty()) {
                n = r->stack_.back();
                r->stack_.pop_back();
                Node *nn = r->nodes_[n];
                if (nn->visited) continue;

                nn->visited = true;
                r->deltab_.push_back(n);

                HASH_FOR_EACH(w, nn->in) {
                    Node *nw = r->nodes_[w];
                    if (!nw->visited && lower_bound < nw->rank) {
                        r->stack_.push_back(w);
                    }
                }
            }
        }

        static void Reorder(graph_cycles::Rep *r) {
            Sort(r->nodes_, &r->deltab_);
            Sort(r->nodes_, &r->deltaf_);

            // Adds contents of delta lists to list_ (backwards deltas first).
            r->list_.clear();
            MoveToList(r, &r->deltab_, &r->list_);
            MoveToList(r, &r->deltaf_, &r->list_);

            // Produce sorted list of all ranks that will be reassigned.
            r->merged_.resize(r->deltab_.size() + r->deltaf_.size());
            std::merge(r->deltab_.begin(), r->deltab_.end(),
                       r->deltaf_.begin(), r->deltaf_.end(),
                       r->merged_.begin());

            // Assign the ranks in order to the collected list.
            for (uint32_t i = 0; i < r->list_.size(); i++) {
                r->nodes_[r->list_[i]]->rank = r->merged_[i];
            }
        }

        static void Sort(const Vec<Node *> &nodes, Vec<int32_t> *delta) {
            struct ByRank {
                const Vec<Node *> *nodes;

                bool operator()(int32_t a, int32_t b) const {
                    return (*nodes)[a]->rank < (*nodes)[b]->rank;
                }
            };
            ByRank cmp;
            cmp.nodes = &nodes;
            std::sort(delta->begin(), delta->end(), cmp);
        }

        static void MoveToList(
                graph_cycles::Rep *r, Vec<int32_t> *src, Vec<int32_t> *dst) {
            for (auto &v : *src) {
                int32_t w = v;
                v = r->nodes_[w]->rank;         // Replace v entry with its rank
                r->nodes_[w]->visited = false;  // Prepare for future DFS calls
                dst->push_back(w);
            }
        }

        int graph_cycles::find_path(graph_id idx, graph_id idy, int max_path_len,
                                  graph_id path[]) const {
            Rep *r = rep_;
            if (FindNode(r, idx) == nullptr || FindNode(r, idy) == nullptr) return 0;
            const int32_t x = NodeIndex(idx);
            const int32_t y = NodeIndex(idy);

            // Forward depth first search starting at x until we hit y.
            // As we descend into a node, we push it onto the path.
            // As we leave a node, we remove it from the path.
            int path_len = 0;

            NodeSet seen;
            r->stack_.clear();
            r->stack_.push_back(x);
            while (!r->stack_.empty()) {
                int32_t n = r->stack_.back();
                r->stack_.pop_back();
                if (n < 0) {
                    // Marker to indicate that we are leaving a node
                    path_len--;
                    continue;
                }

                if (path_len < max_path_len) {
                    path[path_len] = MakeId(n, rep_->nodes_[n]->version);
                }
                path_len++;
                r->stack_.push_back(-1);  // Will remove tentative path entry

                if (n == y) {
                    return path_len;
                }

                HASH_FOR_EACH(w, r->nodes_[n]->out) {
                    if (seen.insert(w)) {
                        r->stack_.push_back(w);
                    }
                }
            }

            return 0;
        }

        bool graph_cycles::is_reachable(graph_id x, graph_id y) const {
            return find_path(x, y, 0, nullptr) > 0;
        }

        void graph_cycles::update_stack_trace(graph_id id, int priority,
                                           int (*get_stack_trace)(void **stack, int)) {
            Node *n = FindNode(rep_, id);
            if (n == nullptr || n->priority >= priority) {
                return;
            }
            n->nstack = (*get_stack_trace)(n->stack, ABEL_ARRAYSIZE(n->stack));
            n->priority = priority;
        }

        int graph_cycles::get_stack_trace(graph_id id, void ***ptr) {
            Node *n = FindNode(rep_, id);
            if (n == nullptr) {
                *ptr = nullptr;
                return 0;
            } else {
                *ptr = n->stack;
                return n->nstack;
            }
        }

    }  // namespace thread_internal

}  // namespace abel

#endif  // ABEL_LOW_LEVEL_ALLOC_MISSING
