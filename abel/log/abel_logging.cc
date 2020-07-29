//
// Created by liyinbin on 2020/4/12.
//

#include <abel/log/abel_logging.h>

namespace abel {

    std::shared_ptr<abel::logger> log_singleton::_log_ptr;

    void create_log_ptr() {
        log_singleton::_log_ptr = abel::stdout_color_mt("abel");
    }
} //namespace abel
