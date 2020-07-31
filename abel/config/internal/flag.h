//
//

#ifndef ABEL_CONFIG_INTERNAL_FLAG_H_
#define ABEL_CONFIG_INTERNAL_FLAG_H_

#include <atomic>
#include <cstring>

#include <abel/thread/thread_annotations.h>
#include <abel/config/internal/command_line_flag.h>
#include <abel/config/internal/registry.h>
#include <abel/memory/memory.h>
#include <abel/strings/str_cat.h>
#include <abel/thread/mutex.h>

namespace abel {

    namespace flags_internal {

        constexpr int64_t atomic_init() { return 0xababababababababll; }

        template<typename T>
        class abel_flag;

        template<typename T>
        class flag_state : public flags_internal::flag_state_interface {
        public:
            flag_state(abel_flag<T> *flag, T &&cur, bool modified, bool on_command_line,
                       int64_t counter)
                    : flag_(flag),
                      cur_value_(std::move(cur)),
                      modified_(modified),
                      on_command_line_(on_command_line),
                      counter_(counter) {}

            ~flag_state() override = default;

        private:
            friend class abel_flag<T>;

            // Restores the flag to the saved state.
            void restore() const override;

            // abel_flag and saved flag data.
            abel_flag<T> *flag_;
            T cur_value_;
            bool modified_;
            bool on_command_line_;
            int64_t counter_;
        };

// This is help argument for abel::abel_flag encapsulating the string literal pointer
// or pointer to function generating it as well as enum descriminating two
// cases.
        using help_gen_func = std::string (*)();

        union flag_help_src {
            constexpr explicit flag_help_src(const char *help_msg) : literal(help_msg) {}

            constexpr explicit flag_help_src(help_gen_func help_gen) : gen_func(help_gen) {}

            const char *literal;
            help_gen_func gen_func;
        };

        enum class flag_help_src_kind : int8_t {
            kLiteral, kGenFunc
        };

        struct help_init_arg {
            flag_help_src source;
            flag_help_src_kind kind;
        };

        extern const char kStrippedFlagHelp[];

// help_constexpr_wrap is used by struct abel_flag_help_gen_for_##name generated by
// ABEL_FLAG macro. It is only used to silence the compiler in the case where
// help message expression is not constexpr and does not have type const char*.
// If help message expression is indeed constexpr const char* help_constexpr_wrap
// is just a trivial identity function.
        template<typename T>
        const char *help_constexpr_wrap(const T &) {
            return nullptr;
        }

        constexpr const char *help_constexpr_wrap(const char *p) { return p; }

        constexpr const char *help_constexpr_wrap(char *p) { return p; }

// These two help_arg overloads allows us to select at compile time one of two
// way to pass Help argument to abel::abel_flag. We'll be passing
// abel_flag_help_gen_for_##name as T and integer 0 as a single argument to prefer
// first overload if possible. If T::Const is evaluatable on constexpr
// context (see non template int parameter below) we'll choose first overload.
// In this case the help message expression is immediately evaluated and is used
// to construct the abel::abel_flag. No additionl code is generated by ABEL_FLAG.
// Otherwise SFINAE kicks in and first overload is dropped from the
// consideration, in which case the second overload will be used. The second
// overload does not attempt to evaluate the help message expression
// immediately and instead delays the evaluation by returing the function
// pointer (&T::NonConst) genering the help message when necessary. This is
// evaluatable in constexpr context, but the cost is an extra function being
// generated in the ABEL_FLAG code.
        template<typename T, int = (T::Const(), 1)>
        constexpr flags_internal::help_init_arg help_arg(int) {
            return {flags_internal::flag_help_src(T::Const()),
                    flags_internal::flag_help_src_kind::kLiteral};
        }

        template<typename T>
        constexpr flags_internal::help_init_arg help_arg(char) {
            return {flags_internal::flag_help_src(&T::NonConst),
                    flags_internal::flag_help_src_kind::kGenFunc};
        }

// Signature for the function generating the initial flag value (usually
// based on default value supplied in flag's definition)
        using flag_dflt_gen_func = void *(*)();

        union flag_default_src {
            constexpr explicit flag_default_src(flag_dflt_gen_func gen_func_arg)
                    : gen_func(gen_func_arg) {}

            void *dynamic_value;
            flag_dflt_gen_func gen_func;
        };

        enum class flag_dfault_src_kind : int8_t {
            kDynamicValue, kGenFunc
        };

// Signature for the mutation callback used by watched Flags
// The callback is noexcept.
// TODO(rogeeff): add noexcept after C++17 support is added.
        using flag_callback = void (*)();

        struct dyn_value_deleter {
            void operator()(void *ptr) const { remove(op, ptr); }

            const flag_op_fn op;
        };

// The class encapsulates the abel_flag's data and safe access to it.
        class flag_impl {
        public:
            constexpr flag_impl(const char *name, const char *filename,
                                const flags_internal::flag_op_fn op,
                                const flags_internal::flag_marshalling_op_fn marshalling_op,
                                const help_init_arg help,
                                const flags_internal::flag_dflt_gen_func default_value_gen)
                    : name_(name),
                      filename_(filename),
                      op_(op),
                      marshalling_op_(marshalling_op),
                      help_(help.source),
                      help_source_kind_(help.kind),
                      def_kind_(flags_internal::flag_dfault_src_kind::kGenFunc),
                      default_src_(default_value_gen),
                      data_guard_{} {}

            // Forces destruction of the abel_flag's data.
            void destroy();

            // Constant access methods
            abel::string_view name() const;

            std::string file_name() const;

            std::string help() const;

            bool is_modified() const ABEL_LOCKS_EXCLUDED(*data_guard());

            bool is_specified_on_command_line() const ABEL_LOCKS_EXCLUDED(*data_guard());

            std::string default_value() const ABEL_LOCKS_EXCLUDED(*data_guard());

            std::string current_value() const ABEL_LOCKS_EXCLUDED(*data_guard());

            void read(void *dst, const flags_internal::flag_op_fn dst_op) const
            ABEL_LOCKS_EXCLUDED(*data_guard());

            // Attempts to parse supplied `value` std::string. If parsing is successful, then
            // it replaces `dst` with the new value.
            bool try_parse(void **dst, abel::string_view value, std::string *err) const
            ABEL_EXCLUSIVE_LOCKS_REQUIRED(*data_guard());

            template<typename T>
            bool atomic_get(T *v) const {
                const int64_t r = atomic_.load(std::memory_order_acquire);
                if (r != flags_internal::atomic_init()) {
                    std::memcpy(v, &r, sizeof(T));
                    return true;
                }

                return false;
            }

            // Mutating access methods
            void write(const void *src, const flags_internal::flag_op_fn src_op)
            ABEL_LOCKS_EXCLUDED(*data_guard());

            bool set_from_string(abel::string_view value, flag_setting_mode set_mode,
                                 value_source source, std::string *err)
            ABEL_LOCKS_EXCLUDED(*data_guard());

            // If possible, updates copy of the abel_flag's value that is stored in an
            // atomic word.
            void store_atomic() ABEL_EXCLUSIVE_LOCKS_REQUIRED(*data_guard());

            // Interfaces to operate on callbacks.
            void set_callback(const flags_internal::flag_callback mutation_callback)
            ABEL_LOCKS_EXCLUDED(*data_guard());

            void invoke_callback() const ABEL_EXCLUSIVE_LOCKS_REQUIRED(*data_guard());

            // Interfaces to save/restore mutable flag data
            template<typename T>
            std::unique_ptr<flags_internal::flag_state_interface> save_state(
                    abel_flag<T> *flag) const ABEL_LOCKS_EXCLUDED(*data_guard()) {
                T &&cur_value = flag->get();
                abel::mutex_lock l(data_guard());

                return abel::make_unique<flags_internal::flag_state<T>>(
                        flag, std::move(cur_value), modified_, on_command_line_, counter_);
            }

            bool restore_state(const void *value, bool modified, bool on_command_line,
                               int64_t counter) ABEL_LOCKS_EXCLUDED(*data_guard());

            // Value validation interfaces.
            void check_default_value_parsing_roundtrip() const
            ABEL_LOCKS_EXCLUDED(*data_guard());

            bool validate_input_value(abel::string_view value) const
            ABEL_LOCKS_EXCLUDED(*data_guard());

        private:
            // Lazy initialization of the abel_flag's data.
            void init();

            // Ensures that the lazily initialized data is initialized,
            // and returns pointer to the mutex guarding flags data.
            abel::mutex *data_guard() const ABEL_LOCK_RETURNED((abel::mutex *) &data_guard_);

            // Returns heap allocated value of type T initialized with default value.
            std::unique_ptr<void, dyn_value_deleter> make_init_value() const
            ABEL_EXCLUSIVE_LOCKS_REQUIRED(*data_guard());

            // Immutable abel_flag's data.
            // Constant configuration for a particular flag.
            const char *const name_;      // Flags name passed to ABEL_FLAG as second arg.
            const char *const filename_;  // The file name where ABEL_FLAG resides.
            const flag_op_fn op_;           // Type-specific handler.
            const flag_marshalling_op_fn marshalling_op_;  // Marshalling ops handler.
            const flag_help_src help_;  // Help message literal or function to generate it.
            // Indicates if help message was supplied as literal or generator func.
            const flag_help_src_kind help_source_kind_;

            // Indicates that the abel_flag state is initialized.
            std::atomic<bool> inited_{false};
            // Mutable abel_flag state (guarded by data_guard_).
            // Additional bool to protect against multiple concurrent constructions
            // of `data_guard_`.
            bool is_data_guard_inited_ = false;
            // Has flag value been modified?
            bool modified_ ABEL_GUARDED_BY(*data_guard()) = false;
            // Specified on command line.
            bool on_command_line_ ABEL_GUARDED_BY(*data_guard()) = false;
            // If def_kind_ == kDynamicValue, default_src_ holds a dynamically allocated
            // value.
            flag_dfault_src_kind def_kind_ ABEL_GUARDED_BY(*data_guard());
            // Either a pointer to the function generating the default value based on the
            // value specified in ABEL_FLAG or pointer to the dynamically set default
            // value via set_command_line_option_with_mode. def_kind_ is used to distinguish
            // these two cases.
            flag_default_src default_src_ ABEL_GUARDED_BY(*data_guard());
            // Lazily initialized pointer to current value
            void *cur_ ABEL_GUARDED_BY(*data_guard()) = nullptr;
            // Mutation counter
            int64_t counter_ ABEL_GUARDED_BY(*data_guard()) = 0;
            // For some types, a copy of the current value is kept in an atomically
            // accessible field.
            std::atomic<int64_t> atomic_{flags_internal::atomic_init()};

            struct callback_data {
                flag_callback func;
                abel::mutex guard;  // Guard for concurrent callback invocations.
            };
            callback_data *callback_data_ ABEL_GUARDED_BY(*data_guard()) = nullptr;
            // This is reserved space for an abel::mutex to guard flag data. It will be
            // initialized in flag_impl::Init via placement new.
            // We can't use "abel::mutex data_guard_", since this class is not literal.
            // We do not want to use "abel::mutex* data_guard_", since this would require
            // heap allocation during initialization, which is both slows program startup
            // and can fail. Using reserved space + placement new allows us to avoid both
            // problems.
            alignas(abel::mutex) mutable char data_guard_[sizeof(abel::mutex)];
        };

// This is "unspecified" implementation of abel::abel_flag<T> type.
        template<typename T>
        class abel_flag final : public flags_internal::command_line_flag {
        public:
            constexpr abel_flag(const char *name, const char *filename,
                                const flags_internal::flag_marshalling_op_fn marshalling_op,
                                const flags_internal::help_init_arg help,
                                const flags_internal::flag_dflt_gen_func default_value_gen)
                    : impl_(name, filename, &flags_internal::flag_ops<T>, marshalling_op, help,
                            default_value_gen) {}

            T get() const {
                // See implementation notes in command_line_flag::get().
                union U {
                    T value;

                    U() {}

                    ~U() { value.~T(); }
                };
                U u;

                impl_.read(&u.value, &flags_internal::flag_ops<T>);
                return std::move(u.value);
            }

            bool atomic_get(T *v) const { return impl_.atomic_get(v); }

            void set(const T &v) { impl_.write(&v, &flags_internal::flag_ops<T>); }

            void set_callback(const flags_internal::flag_callback mutation_callback) {
                impl_.set_callback(mutation_callback);
            }

            // command_line_flag interface
            abel::string_view name() const override { return impl_.name(); }

            std::string file_name() const override { return impl_.file_name(); }

            abel::string_view type_name() const override { return abel::string_view(""); }

            std::string help() const override { return impl_.help(); }

            bool is_modified() const override { return impl_.is_modified(); }

            bool is_specified_on_command_line() const override {
                return impl_.is_specified_on_command_line();
            }

            std::string default_value() const override { return impl_.default_value(); }

            std::string current_value() const override { return impl_.current_value(); }

            bool validate_input_value(abel::string_view value) const override {
                return impl_.validate_input_value(value);
            }

            // Interfaces to save and restore flags to/from persistent state.
            // Returns current flag state or nullptr if flag does not support
            // saving and restoring a state.
            std::unique_ptr<flags_internal::flag_state_interface> save_state() override {
                return impl_.save_state(this);
            }

            // Restores the flag state to the supplied state object. If there is
            // nothing to restore returns false. Otherwise returns true.
            bool restore_state(const flags_internal::flag_state<T> &flag_state) {
                return impl_.restore_state(&flag_state.cur_value_, flag_state.modified_,
                                           flag_state.on_command_line_, flag_state.counter_);
            }

            bool set_from_string(abel::string_view value,
                                 flags_internal::flag_setting_mode set_mode,
                                 flags_internal::value_source source,
                                 std::string *error) override {
                return impl_.set_from_string(value, set_mode, source, error);
            }

            void check_default_value_parsing_roundtrip() const override {
                impl_.check_default_value_parsing_roundtrip();
            }

        private:
            friend class flag_state<T>;

            void destroy() override { impl_.destroy(); }

            void read(void *dst) const override {
                impl_.read(dst, &flags_internal::flag_ops<T>);
            }

            flags_internal::flag_op_fn type_id() const override {
                return &flags_internal::flag_ops<T>;
            }

            // abel_flag's data
            flag_impl impl_;
        };

        template<typename T>
        ABEL_FORCE_INLINE void flag_state<T>::restore() const {
            if (flag_->restore_state(*this)) {
                DLOG_INFO("Restore saved value of {} to: {}", flag_->name(), flag_->current_value());
            }
        }

// This class facilitates abel_flag object registration and tail expression-based
// flag definition, for example:
// ABEL_FLAG(int, foo, 42, "Foo help").on_update(NotifyFooWatcher);
        template<typename T, bool do_register>
        class flag_registrar {
        public:
            explicit flag_registrar(abel_flag<T> *flag) : flag_(flag) {
                if (do_register) flags_internal::register_command_line_flag(flag_);
            }

            flag_registrar &on_update(flags_internal::flag_callback cb) &&{
                flag_->set_callback(cb);
                return *this;
            }

            // Make the registrar "die" gracefully as a bool on a line where registration
            // happens. Registrar objects are intended to live only as temporary.
            operator bool() const { return true; }  // NOLINT

        private:
            abel_flag<T> *flag_;  // abel_flag being registered (not owned).
        };

// This struct and corresponding overload to MakeDefaultValue are used to
// facilitate usage of {} as default value in ABEL_FLAG macro.
        struct empty_braces {
        };

        template<typename T>
        T *make_from_default_value(T t) {
            return new T(std::move(t));
        }

        template<typename T>
        T *make_from_default_value(empty_braces) {
            return new T;
        }

    }  // namespace flags_internal

}  // namespace abel

#endif  // ABEL_CONFIG_INTERNAL_FLAG_H_
