//
// Created by liyinbin on 2020/2/8.
//

#include <abel/metrics/stop_watcher.h>
#include <abel/metrics/timer.h>

namespace abel {
    namespace metrics {
        stop_watcher::stop_watcher(abel::abel_time start,
                                   std::shared_ptr<timer> recorder)
                : _start(start), _recorder(std::move(recorder)) {

        }

        void stop_watcher::stop() {
            if (_recorder != nullptr) {
                _recorder->record(_start);
            }
        }

    } //namespace metrics
} //namespace abel
