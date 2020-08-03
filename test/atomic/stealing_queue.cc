//
// Created by liyinbin on 2020/8/3.
//

#include <algorithm>                        // std::sort
#include <gtest/gtest.h>
#include <abel/chrono/clock.h>
#include <abel/thread/mutex.h>
#include <abel/atomic/stealing_queue.h>

namespace {
    typedef size_t value_type;
    bool g_stop = false;
    const size_t N = 1024*512;
    const size_t CAP = 8;
    abel::mutex mutex;

    void* steal_thread(void* arg) {
        std::vector<value_type> *stolen = new std::vector<value_type>;
        stolen->reserve(N);
        abel::stealing_queue<value_type> *q =
                (abel::stealing_queue<value_type>*)arg;
        value_type val;
        while (!g_stop) {
            if (q->steal(&val)) {
                stolen->push_back(val);
            } else {
                asm volatile("pause\n": : :"memory");
            }
        }
        return stolen;
    }

    void* push_thread(void* arg) {
        size_t npushed = 0;
        value_type seed = 0;
        abel::stealing_queue<value_type> *q =
                (abel::stealing_queue<value_type>*)arg;
        while (true) {
            mutex.lock();
            const bool pushed = q->push(seed);
            mutex.unlock();
            if (pushed) {
                ++seed;
                if (++npushed == N) {
                    g_stop = true;
                    break;
                }
            }
        }
        return NULL;
    }

    void* pop_thread(void* arg) {
        std::vector<value_type> *popped = new std::vector<value_type>;
        popped->reserve(N);
        abel::stealing_queue<value_type> *q =
                (abel::stealing_queue<value_type>*)arg;
        while (!g_stop) {
            value_type val;
            mutex.lock();
            const bool res = q->pop(&val);
            mutex.unlock();
            if (res) {
                popped->push_back(val);
            }
        }
        return popped;
    }


    TEST(WSQTest, sanity) {
        abel::stealing_queue<value_type> q;
        ASSERT_EQ(0, q.init(CAP));
        pthread_t rth[8];
        pthread_t wth, pop_th;
        for (size_t i = 0; i < ABEL_ARRAY_SIZE(rth); ++i) {
            ASSERT_EQ(0, pthread_create(&rth[i], NULL, steal_thread, &q));
        }
        ASSERT_EQ(0, pthread_create(&wth, NULL, push_thread, &q));
        ASSERT_EQ(0, pthread_create(&pop_th, NULL, pop_thread, &q));

        std::vector<value_type> values;
        values.reserve(N);
        size_t nstolen = 0, npopped = 0;
        for (size_t i = 0; i < ABEL_ARRAY_SIZE(rth); ++i) {
            std::vector<value_type>* res = NULL;
            pthread_join(rth[i], (void**)&res);
            for (size_t j = 0; j < res->size(); ++j, ++nstolen) {
                values.push_back((*res)[j]);
            }
        }
        pthread_join(wth, NULL);
        std::vector<value_type>* res = NULL;
        pthread_join(pop_th, (void**)&res);
        for (size_t j = 0; j < res->size(); ++j, ++npopped) {
            values.push_back((*res)[j]);
        }

        value_type val;
        while (q.pop(&val)) {
            values.push_back(val);
        }

        std::sort(values.begin(), values.end());
        values.resize(std::unique(values.begin(), values.end()) - values.begin());

        ASSERT_EQ(N, values.size());
        for (size_t i = 0; i < N; ++i) {
            ASSERT_EQ(i, values[i]);
        }
        std::cout << "stolen=" << nstolen
                  << " popped=" << npopped
                  << " left=" << (N - nstolen - npopped)  << std::endl;
    }
} // namespace
