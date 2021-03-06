//
// This file is a Posix-specific part of spinlock_wait.cc

#include <sched.h>
#include <atomic>
#include <ctime>
#include <cerrno>

#include <abel/thread/internal/scheduling_mode.h>
#include <abel/base/profile.h>

extern "C" {

ABEL_WEAK void abel_internal_spin_lock_delay(
        std::atomic<uint32_t> * /* lock_word */, uint32_t /* value */, int loop,
        abel::thread_internal::SchedulingMode /* mode */) {
    int save_errno = errno;
    if (loop == 0) {
    } else if (loop == 1) {
        sched_yield();
    } else {
        struct timespec tm;
        tm.tv_sec = 0;
        tm.tv_nsec = abel::thread_internal::spin_lock_suggested_delay_ns(loop);
        nanosleep(&tm, nullptr);
    }
    errno = save_errno;
}

ABEL_WEAK void abel_internal_spin_lock_wake(
        std::atomic<uint32_t> * /* lock_word */, bool /* all */) {}

}  // extern "C"
