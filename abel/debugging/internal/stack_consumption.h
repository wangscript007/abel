//
//

// Helper function for measuring stack consumption of signal handlers.

#ifndef ABEL_DEBUGGING_INTERNAL_STACK_CONSUMPTION_H_
#define ABEL_DEBUGGING_INTERNAL_STACK_CONSUMPTION_H_

#include <abel/base/profile.h>

// The code in this module is not portable.
// Use this feature test macro to detect its availability.
#ifdef ABEL_INTERNAL_HAVE_DEBUGGING_STACK_CONSUMPTION
#error ABEL_INTERNAL_HAVE_DEBUGGING_STACK_CONSUMPTION cannot be set directly
#elif !defined(__APPLE__) && !defined(_WIN32) && \
    (defined(__i386__) || defined(__x86_64__) || defined(__ppc__))
#define ABEL_INTERNAL_HAVE_DEBUGGING_STACK_CONSUMPTION 1

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace debugging_internal {

// Returns the stack consumption in bytes for the code exercised by
// signal_handler.  To measure stack consumption, signal_handler is registered
// as a signal handler, so the code that it exercises must be async-signal
// safe.  The argument of signal_handler is an implementation detail of signal
// handlers and should ignored by the code for signal_handler.  Use global
// variables to pass information between your test code and signal_handler.
int GetSignalHandlerStackConsumption(void (*signal_handler)(int));

}  // namespace debugging_internal
ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_INTERNAL_HAVE_DEBUGGING_STACK_CONSUMPTION

#endif  // ABEL_DEBUGGING_INTERNAL_STACK_CONSUMPTION_H_
