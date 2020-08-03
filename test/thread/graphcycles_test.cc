//

#include <abel/thread/internal/graphcycles.h>

#include <map>
#include <random>
#include <unordered_set>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <abel/log/logging.h>
#include <abel/base/profile.h>

namespace abel {

    namespace thread_internal {

// We emulate a graph_cycles object with a node vector and an edge vector.
// We then compare the two implementations.

        using Nodes = std::vector<int>;
        struct Edge {
            int from;
            int to;
        };
        using Edges = std::vector<Edge>;
        using RandomEngine = std::mt19937_64;

// Mapping from integer index to graph_id.
        typedef std::map<int, graph_id> IdMap;

        static graph_id Get(const IdMap &id, int num) {
            auto iter = id.find(num);
            return (iter == id.end()) ? invalid_graphId() : iter->second;
        }

// Return whether "to" is reachable from "from".
        static bool is_reachable(Edges *edges, int from, int to,
                                std::unordered_set<int> *seen) {
            seen->insert(from);     // we are investigating "from"; don't do it again
            if (from == to) return true;
            for (const auto &edge : *edges) {
                if (edge.from == from) {
                    if (edge.to == to) {  // success via edge directly
                        return true;
                    } else if (seen->find(edge.to) == seen->end() &&  // success via edge
                               is_reachable(edges, edge.to, to, seen)) {
                        return true;
                    }
                }
            }
            return false;
        }

        static void PrintEdges(Edges *edges) {
            DLOG_INFO("EDGES ({})", edges->size());
            for (const auto &edge : *edges) {
                int a = edge.from;
                int b = edge.to;
                DLOG_INFO("{} {}", a, b);
            }
            DLOG_INFO("---");
        }

        static void PrintGCEdges(Nodes *nodes, const IdMap &id, graph_cycles *gc) {
            DLOG_INFO("GC EDGES");
            for (int a : *nodes) {
                for (int b : *nodes) {
                    if (gc->has_edge(Get(id, a), Get(id, b))) {
                        DLOG_INFO("{} {}", a, b);
                    }
                }
            }
            DLOG_INFO("---");
        }

        static void PrintTransitiveClosure(Nodes *nodes, Edges *edges) {
            DLOG_INFO("Transitive closure");
            for (int a : *nodes) {
                for (int b : *nodes) {
                    std::unordered_set<int> seen;
                    if (is_reachable(edges, a, b, &seen)) {
                        DLOG_INFO("{} {}", a, b);
                    }
                }
            }
            DLOG_INFO("---");
        }

        static void PrintGCTransitiveClosure(Nodes *nodes, const IdMap &id,
                                             graph_cycles *gc) {
            DLOG_INFO("GC Transitive closure");
            for (int a : *nodes) {
                for (int b : *nodes) {
                    if (gc->is_reachable(Get(id, a), Get(id, b))) {
                        DLOG_INFO("{} {}", a, b);
                    }
                }
            }
            DLOG_INFO("---");
        }

        static void CheckTransitiveClosure(Nodes *nodes, Edges *edges, const IdMap &id,
                                           graph_cycles *gc) {
            std::unordered_set<int> seen;
            for (const auto &a : *nodes) {
                for (const auto &b : *nodes) {
                    seen.clear();
                    bool gc_reachable = gc->is_reachable(Get(id, a), Get(id, b));
                    bool reachable = is_reachable(edges, a, b, &seen);
                    if (gc_reachable != reachable) {
                        PrintEdges(edges);
                        PrintGCEdges(nodes, id, gc);
                        PrintTransitiveClosure(nodes, edges);
                        PrintGCTransitiveClosure(nodes, id, gc);
                        DLOG_CRITICAL("gc_reachable %s reachable {} a {} b {}",
                                     gc_reachable ? "true" : "false",
                                     reachable ? "true" : "false", a, b);
                    }
                }
            }
        }

        static void CheckEdges(Nodes *nodes, Edges *edges, const IdMap &id,
                               graph_cycles *gc) {
            size_t count = 0;
            for (const auto &edge : *edges) {
                int a = edge.from;
                int b = edge.to;
                if (!gc->has_edge(Get(id, a), Get(id, b))) {
                    PrintEdges(edges);
                    PrintGCEdges(nodes, id, gc);
                    DLOG_CRITICAL("!gc->has_edge({}, {})", a, b);
                }
            }
            for (const auto &a : *nodes) {
                for (const auto &b : *nodes) {
                    if (gc->has_edge(Get(id, a), Get(id, b))) {
                        count++;
                    }
                }
            }
            if (count != edges->size()) {
                PrintEdges(edges);
                PrintGCEdges(nodes, id, gc);
                DLOG_CRITICAL("edges->size() {}  count {}", edges->size(), count);
            }
        }

        static void check_invariants(const graph_cycles &gc) {
            if (ABEL_UNLIKELY(!gc.check_invariants()))
                DLOG_CRITICAL("check_invariants");
        }

// Returns the index of a randomly chosen node in *nodes.
// Requires *nodes be non-empty.
        static int RandomNode(RandomEngine *rng, Nodes *nodes) {
            std::uniform_int_distribution<int> uniform(0, nodes->size() - 1);
            return uniform(*rng);
        }

// Returns the index of a randomly chosen edge in *edges.
// Requires *edges be non-empty.
        static int RandomEdge(RandomEngine *rng, Edges *edges) {
            std::uniform_int_distribution<int> uniform(0, edges->size() - 1);
            return uniform(*rng);
        }

// Returns the index of edge (from, to) in *edges or -1 if it is not in *edges.
        static int EdgeIndex(Edges *edges, int from, int to) {
            size_t i = 0;
            while (i != edges->size() &&
                   ((*edges)[i].from != from || (*edges)[i].to != to)) {
                i++;
            }
            return i == edges->size() ? -1 : i;
        }

        TEST(graph_cycles, RandomizedTest) {
            int next_node = 0;
            Nodes nodes;
            Edges edges;   // from, to
            IdMap id;
            graph_cycles graph_cycles;
            static const int kMaxNodes = 7;  // use <= 7 nodes to keep test short
            static const int kDataOffset = 17;  // an offset to the node-specific data
            int n = 100000;
            int op = 0;
            RandomEngine rng(testing::UnitTest::GetInstance()->random_seed());
            std::uniform_int_distribution<int> uniform(0, 5);

            auto ptr = [](intptr_t i) {
                return reinterpret_cast<void *>(i + kDataOffset);
            };

            for (int iter = 0; iter != n; iter++) {
                for (const auto &node : nodes) {
                    ASSERT_EQ(graph_cycles.ptr(Get(id, node)), ptr(node)) << " node " << node;
                }
                CheckEdges(&nodes, &edges, id, &graph_cycles);
                CheckTransitiveClosure(&nodes, &edges, id, &graph_cycles);
                op = uniform(rng);
                switch (op) {
                    case 0:     // Add a node
                        if (nodes.size() < kMaxNodes) {
                            int new_node = next_node++;
                            graph_id new_gnode = graph_cycles.get_id(ptr(new_node));
                            ASSERT_NE(new_gnode, invalid_graphId());
                            id[new_node] = new_gnode;
                            ASSERT_EQ(ptr(new_node), graph_cycles.ptr(new_gnode));
                            nodes.push_back(new_node);
                        }
                        break;

                    case 1:    // Remove a node
                        if (nodes.size() > 0) {
                            int node_index = RandomNode(&rng, &nodes);
                            int node = nodes[node_index];
                            nodes[node_index] = nodes.back();
                            nodes.pop_back();
                            graph_cycles.remove_node(ptr(node));
                            ASSERT_EQ(graph_cycles.ptr(Get(id, node)), nullptr);
                            id.erase(node);
                            size_t i = 0;
                            while (i != edges.size()) {
                                if (edges[i].from == node || edges[i].to == node) {
                                    edges[i] = edges.back();
                                    edges.pop_back();
                                } else {
                                    i++;
                                }
                            }
                        }
                        break;

                    case 2:   // Add an edge
                        if (nodes.size() > 0) {
                            int from = RandomNode(&rng, &nodes);
                            int to = RandomNode(&rng, &nodes);
                            if (EdgeIndex(&edges, nodes[from], nodes[to]) == -1) {
                                if (graph_cycles.insert_edge(id[nodes[from]], id[nodes[to]])) {
                                    Edge new_edge;
                                    new_edge.from = nodes[from];
                                    new_edge.to = nodes[to];
                                    edges.push_back(new_edge);
                                } else {
                                    std::unordered_set<int> seen;
                                    ASSERT_TRUE(is_reachable(&edges, nodes[to], nodes[from], &seen))
                                                                << "Edge " << nodes[to] << "->" << nodes[from];
                                }
                            }
                        }
                        break;

                    case 3:    // Remove an edge
                        if (edges.size() > 0) {
                            int i = RandomEdge(&rng, &edges);
                            int from = edges[i].from;
                            int to = edges[i].to;
                            ASSERT_EQ(i, EdgeIndex(&edges, from, to));
                            edges[i] = edges.back();
                            edges.pop_back();
                            ASSERT_EQ(-1, EdgeIndex(&edges, from, to));
                            graph_cycles.remove_edge(id[from], id[to]);
                        }
                        break;

                    case 4:   // Check a path
                        if (nodes.size() > 0) {
                            int from = RandomNode(&rng, &nodes);
                            int to = RandomNode(&rng, &nodes);
                            graph_id path[2 * kMaxNodes];
                            int path_len = graph_cycles.find_path(id[nodes[from]], id[nodes[to]],
                                                                 ABEL_ARRAY_SIZE(path), path);
                            std::unordered_set<int> seen;
                            bool reachable = is_reachable(&edges, nodes[from], nodes[to], &seen);
                            bool gc_reachable =
                                    graph_cycles.is_reachable(Get(id, nodes[from]), Get(id, nodes[to]));
                            ASSERT_EQ(path_len != 0, reachable);
                            ASSERT_EQ(path_len != 0, gc_reachable);
                            // In the following line, we add one because a node can appear
                            // twice, if the path is from that node to itself, perhaps via
                            // every other node.
                            ASSERT_LE(path_len, kMaxNodes + 1);
                            if (path_len != 0) {
                                ASSERT_EQ(id[nodes[from]], path[0]);
                                ASSERT_EQ(id[nodes[to]], path[path_len - 1]);
                                for (int i = 1; i < path_len; i++) {
                                    ASSERT_TRUE(graph_cycles.has_edge(path[i - 1], path[i]));
                                }
                            }
                        }
                        break;

                    case 5:  // Check invariants
                        check_invariants(graph_cycles);
                        break;

                    default:
                        DLOG_CRITICAL("op {}", op);
                }

                // Very rarely, test graph expansion by adding then removing many nodes.
                std::bernoulli_distribution one_in_1024(1.0 / 1024);
                if (one_in_1024(rng)) {
                    CheckEdges(&nodes, &edges, id, &graph_cycles);
                    CheckTransitiveClosure(&nodes, &edges, id, &graph_cycles);
                    for (int i = 0; i != 256; i++) {
                        int new_node = next_node++;
                        graph_id new_gnode = graph_cycles.get_id(ptr(new_node));
                        ASSERT_NE(invalid_graphId(), new_gnode);
                        id[new_node] = new_gnode;
                        ASSERT_EQ(ptr(new_node), graph_cycles.ptr(new_gnode));
                        for (const auto &node : nodes) {
                            ASSERT_NE(node, new_node);
                        }
                        nodes.push_back(new_node);
                    }
                    for (int i = 0; i != 256; i++) {
                        ASSERT_GT(nodes.size(), 0);
                        int node_index = RandomNode(&rng, &nodes);
                        int node = nodes[node_index];
                        nodes[node_index] = nodes.back();
                        nodes.pop_back();
                        graph_cycles.remove_node(ptr(node));
                        id.erase(node);
                        size_t j = 0;
                        while (j != edges.size()) {
                            if (edges[j].from == node || edges[j].to == node) {
                                edges[j] = edges.back();
                                edges.pop_back();
                            } else {
                                j++;
                            }
                        }
                    }
                    check_invariants(graph_cycles);
                }
            }
        }

        class GraphCyclesTest : public ::testing::Test {
        public:
            IdMap id_;
            graph_cycles g_;

            static void *Ptr(int i) {
                return reinterpret_cast<void *>(static_cast<uintptr_t>(i));
            }

            static int Num(void *ptr) {
                return static_cast<int>(reinterpret_cast<uintptr_t>(ptr));
            }

            // Test relies on ith NewNode() call returning Node numbered i
            GraphCyclesTest() {
                for (int i = 0; i < 100; i++) {
                    id_[i] = g_.get_id(Ptr(i));
                }
                check_invariants(g_);
            }

            bool AddEdge(int x, int y) {
                return g_.insert_edge(Get(id_, x), Get(id_, y));
            }

            void AddMultiples() {
                // For every node x > 0: add edge to 2*x, 3*x
                for (int x = 1; x < 25; x++) {
                    EXPECT_TRUE(AddEdge(x, 2 * x)) << x;
                    EXPECT_TRUE(AddEdge(x, 3 * x)) << x;
                }
                check_invariants(g_);
            }

            std::string Path(int x, int y) {
                graph_id path[5];
                int np = g_.find_path(Get(id_, x), Get(id_, y), ABEL_ARRAY_SIZE(path), path);
                std::string result;
                for (size_t i = 0; i < static_cast<size_t>(np); i++) {
                    if (i >= ABEL_ARRAY_SIZE(path)) {
                        result += " ...";
                        break;
                    }
                    if (!result.empty()) result.push_back(' ');
                    char buf[20];
                    snprintf(buf, sizeof(buf), "%d", Num(g_.ptr(path[i])));
                    result += buf;
                }
                return result;
            }
        };

        TEST_F(GraphCyclesTest, NoCycle) {
            AddMultiples();
            check_invariants(g_);
        }

        TEST_F(GraphCyclesTest, SimpleCycle) {
            AddMultiples();
            EXPECT_FALSE(AddEdge(8, 4));
            EXPECT_EQ("4 8", Path(4, 8));
            check_invariants(g_);
        }

        TEST_F(GraphCyclesTest, IndirectCycle) {
            AddMultiples();
            EXPECT_TRUE(AddEdge(16, 9));
            check_invariants(g_);
            EXPECT_FALSE(AddEdge(9, 2));
            EXPECT_EQ("2 4 8 16 9", Path(2, 9));
            check_invariants(g_);
        }

        TEST_F(GraphCyclesTest, LongPath) {
            ASSERT_TRUE(AddEdge(2, 4));
            ASSERT_TRUE(AddEdge(4, 6));
            ASSERT_TRUE(AddEdge(6, 8));
            ASSERT_TRUE(AddEdge(8, 10));
            ASSERT_TRUE(AddEdge(10, 12));
            ASSERT_FALSE(AddEdge(12, 2));
            EXPECT_EQ("2 4 6 8 10 ...", Path(2, 12));
            check_invariants(g_);
        }

        TEST_F(GraphCyclesTest, remove_node) {
            ASSERT_TRUE(AddEdge(1, 2));
            ASSERT_TRUE(AddEdge(2, 3));
            ASSERT_TRUE(AddEdge(3, 4));
            ASSERT_TRUE(AddEdge(4, 5));
            g_.remove_node(g_.ptr(id_[3]));
            id_.erase(3);
            ASSERT_TRUE(AddEdge(5, 1));
        }

        TEST_F(GraphCyclesTest, ManyEdges) {
            const int N = 50;
            for (int i = 0; i < N; i++) {
                for (int j = 1; j < N; j++) {
                    ASSERT_TRUE(AddEdge(i, i + j));
                }
            }
            check_invariants(g_);
            ASSERT_TRUE(AddEdge(2 * N - 1, 0));
            check_invariants(g_);
            ASSERT_FALSE(AddEdge(10, 9));
            check_invariants(g_);
        }

    }  // namespace thread_internal

}  // namespace abel
