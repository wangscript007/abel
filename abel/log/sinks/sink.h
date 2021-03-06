//
// Copyright(c) 2015 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#pragma once

#include <abel/log/details/log_msg.h>
#include <abel/log/details/pattern_formatter.h>
#include <abel/log/formatter.h>

namespace abel {
    namespace log {
        namespace sinks {
            class sink {
            public:
                sink()
                        : level_(trace), formatter_(new pattern_formatter("%+")) {
                }

                sink(std::unique_ptr<abel::log::pattern_formatter> formatter)
                        : level_(trace), formatter_(std::move(formatter)) {};

                virtual ~sink() = default;

                virtual void log(const details::log_msg &msg) = 0;

                virtual void flush() = 0;

                virtual void set_pattern(const std::string &pattern) = 0;

                virtual void set_formatter(std::unique_ptr<abel::log::formatter> sink_formatter) = 0;

                bool should_log(level_enum msg_level) const {
                    return msg_level >= level_.load(std::memory_order_relaxed);
                }

                void set_level(abel::log::level_enum log_level) {
                    level_.store(log_level);
                }

                log::level_enum level() const {
                    return static_cast<abel::log::level_enum>(level_.load(std::memory_order_relaxed));
                }

            protected:
                // sink log level - default is all
                level_t level_;

                // sink formatter - default is full format
                std::unique_ptr<abel::log::formatter> formatter_;
            };

        } // namespace sinks
    } //namespace log
} // namespace abel
