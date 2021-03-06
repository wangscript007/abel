//
//
// -----------------------------------------------------------------------------
// File: flag.h
// -----------------------------------------------------------------------------
//
// This header file defines the `abel::abel_flag<T>` type for holding command-line
// flag data, and abstractions to create, get and set such flag data.
//
// It is important to note that this type is **unspecified** (an implementation
// detail) and you do not construct or manipulate actual `abel::abel_flag<T>`
// instances. Instead, you define and declare flags using the
// `ABEL_FLAG()` and `ABEL_DECLARE_FLAG()` macros, and get and set flag values
// using the `abel::get_flag()` and `abel::set_flag()` functions.

#ifndef ABEL_CONFIG_FLAG_H_
#define ABEL_CONFIG_FLAG_H_

#include <abel/base/profile.h>
#include <abel/math/bit_cast.h>
#include <abel/config/config.h>
#include <abel/config/declare.h>
#include <abel/config/internal/command_line_flag.h>
#include <abel/config/internal/flag.h>
#include <abel/config/marshalling.h>
#include <vector>
#include <functional>

namespace abel {

    using command_line_flag = flags_internal::command_line_flag;

    std::vector<command_line_flag *> get_all_flags();

    std::vector<command_line_flag *> get_all_flags_unlock();

    using flag_visitor = std::function<void(command_line_flag *)>;

    void visit_flags_unlock(const flag_visitor &fv);

    void visit_flags(const flag_visitor &fv);


// abel_flag
//
// An `abel::abel_flag` holds a command-line flag value, providing a runtime
// parameter to a binary. Such flags should be defined in the global namespace
// and (preferably) in the module containing the binary's `main()` function.
//
// You should not construct and cannot use the `abel::abel_flag` type directly;
// instead, you should declare flags using the `ABEL_DECLARE_FLAG()` macro
// within a header file, and define your flag using `ABEL_FLAG()` within your
// header's associated `.cc` file. Such flags will be named `FLAGS_name`.
//
// Example:
//
//    .h file
//
//      // Declares usage of a flag named "FLAGS_count"
//      ABEL_DECLARE_FLAG(int, count);
//
//    .cc file
//
//      // Defines a flag named "FLAGS_count" with a default `int` value of 0.
//      ABEL_FLAG(int, count, 0, "Count of items to process");
//
// No public methods of `abel::abel_flag<T>` are part of the abel Flags API.
#if !defined(_MSC_VER) || defined(__clang__)
    template<typename T>
    using abel_flag = flags_internal::abel_flag<T>;
#else
    // MSVC debug builds do not implement initialization with constexpr constructors
    // correctly. To work around this we add a level of indirection, so that the
    // class `abel::abel_flag` contains an `internal::abel_flag*` (instead of being an alias
    // to that class) and dynamically allocates an instance when necessary. We also
    // forward all calls to internal::abel_flag methods via trampoline methods. In this
    // setup the `abel::abel_flag` class does not have constructor and virtual methods,
    // all the data members are public and thus MSVC is able to initialize it at
    // link time. To deal with multiple threads accessing the flag for the first
    // time concurrently we use an atomic boolean indicating if flag object is
    // initialized. We also employ the double-checked locking pattern where the
    // second level of protection is a global mutex, so if two threads attempt to
    // construct the flag concurrently only one wins.
    // This solution is based on a recomendation here:
    // https://developercommunity.visualstudio.com/content/problem/336946/class-with-constexpr-constructor-not-using-static.html?childToView=648454#comment-648454

    namespace flags_internal {
    abel::mutex* get_global_construction_guard();
    }  // namespace flags_internal

    template <typename T>
    class abel_flag {
     public:
      // No constructor and destructor to ensure this is an aggregate type.
      // Visual Studio 2015 still requires the constructor for class to be
      // constexpr initializable.
#if _MSC_VER <= 1900
      constexpr abel_flag(const char* name, const char* filename,
                     const flags_internal::flag_marshalling_op_fn marshalling_op,
                     const flags_internal::help_gen_func help_gen,
                     const flags_internal::flag_dflt_gen_func default_value_gen)
          : name_(name),
            filename_(filename),
            marshalling_op_(marshalling_op),
            help_gen_(help_gen),
            default_value_gen_(default_value_gen),
            inited_(false),
            impl_(nullptr) {}
#endif

      flags_internal::abel_flag<T>* GetImpl() const {
        if (!inited_.load(std::memory_order_acquire)) {
          abel::mutex_lock l(flags_internal::get_global_construction_guard());

          if (inited_.load(std::memory_order_acquire)) {
            return impl_;
          }

          impl_ = new flags_internal::abel_flag<T>(
              name_, filename_, marshalling_op_,
              {flags_internal::flag_help_src(help_gen_),
               flags_internal::flag_help_src_kind::kGenFunc},
              default_value_gen_);
          inited_.store(true, std::memory_order_release);
        }

        return impl_;
      }

      // Public methods of `abel::abel_flag<T>` are NOT part of the abel Flags API.
      bool is_retired() const { return GetImpl()->is_retired(); }
      bool is_abel_flag() const { return GetImpl()->is_abel_flag(); }
      abel::string_view name() const { return GetImpl()->name(); }
      std::string help() const { return GetImpl()->help(); }
      bool is_modified() const { return GetImpl()->is_modified(); }
      bool is_specified_on_command_line() const {
        return GetImpl()->is_specified_on_command_line();
      }
      abel::string_view type_name() const { return GetImpl()->type_name(); }
      std::string file_name() const { return GetImpl()->file_name(); }
      std::string default_value() const { return GetImpl()->default_value(); }
      std::string current_value() const { return GetImpl()->current_value(); }
      template <typename U>
      ABEL_FORCE_INLINE bool is_of_type() const {
        return GetImpl()->template is_of_type<U>();
      }
      T get() const { return GetImpl()->get(); }
      bool atomic_get(T* v) const { return GetImpl()->atomic_get(v); }
      void set(const T& v) { GetImpl()->set(v); }
      void set_callback(const flags_internal::flag_callback mutation_callback) {
        GetImpl()->set_callback(mutation_callback);
      }
      void invoke_callback() { GetImpl()->invoke_callback(); }

      // The data members are logically private, but they need to be public for
      // this to be an aggregate type.
      const char* name_;
      const char* filename_;
      const flags_internal::flag_marshalling_op_fn marshalling_op_;
      const flags_internal::help_gen_func help_gen_;
      const flags_internal::flag_dflt_gen_func default_value_gen_;

      mutable std::atomic<bool> inited_;
      mutable flags_internal::abel_flag<T>* impl_;
    };
#endif

// get_flag()
//
// Returns the value (of type `T`) of an `abel::abel_flag<T>` instance, by value. Do
// not construct an `abel::abel_flag<T>` directly and call `abel::get_flag()`;
// instead, refer to flag's constructed variable name (e.g. `FLAGS_name`).
// Because this function returns by value and not by reference, it is
// thread-safe, but note that the operation may be expensive; as a result, avoid
// `abel::get_flag()` within any tight loops.
//
// Example:
//
//   // FLAGS_count is a abel_flag of type `int`
//   int my_count = abel::get_flag(FLAGS_count);
//
//   // FLAGS_firstname is a abel_flag of type `std::string`
//   std::string first_name = abel::get_flag(FLAGS_firstname);
    template<typename T>
    ABEL_MUST_USE_RESULT T get_flag(const abel::abel_flag<T> &flag) {
#define ABEL_FLAGS_INTERNAL_LOCK_FREE_VALIDATE(BIT) \
  static_assert(                                    \
      !std::is_same<T, BIT>::value,                 \
      "Do not specify explicit template parameters to abel::get_flag");
        ABEL_FLAGS_INTERNAL_FOR_EACH_LOCK_FREE(ABEL_FLAGS_INTERNAL_LOCK_FREE_VALIDATE)
#undef ABEL_FLAGS_INTERNAL_LOCK_FREE_VALIDATE

        return flag.get();
    }

// Overload for `get_flag()` for types that support lock-free reads.
#define ABEL_FLAGS_INTERNAL_LOCK_FREE_EXPORT(T) \
  ABEL_MUST_USE_RESULT T get_flag(const abel::abel_flag<T>& flag);

    ABEL_FLAGS_INTERNAL_FOR_EACH_LOCK_FREE(ABEL_FLAGS_INTERNAL_LOCK_FREE_EXPORT)

#undef ABEL_FLAGS_INTERNAL_LOCK_FREE_EXPORT

// set_flag()
//
// Sets the value of an `abel::abel_flag` to the value `v`. Do not construct an
// `abel::abel_flag<T>` directly and call `abel::set_flag()`; instead, use the
// flag's variable name (e.g. `FLAGS_name`). This function is
// thread-safe, but is potentially expensive. Avoid setting flags in general,
// but especially within performance-critical code.
    template<typename T>
    void set_flag(abel::abel_flag<T> *flag, const T &v) {
        flag->set(v);
    }

// Overload of `set_flag()` to allow callers to pass in a value that is
// convertible to `T`. E.g., use this overload to pass a "const char*" when `T`
// is `std::string`.
    template<typename T, typename V>
    void set_flag(abel::abel_flag<T> *flag, const V &v) {
        T value(v);
        flag->set(value);
    }


}  // namespace abel


// ABEL_FLAG()
//
// This macro defines an `abel::abel_flag<T>` instance of a specified type `T`:
//
//   ABEL_FLAG(T, name, default_value, help);
//
// where:
//
//   * `T` is a supported flag type (see the list of types in `marshalling.h`),
//   * `name` designates the name of the flag (as a global variable
//     `FLAGS_name`),
//   * `default_value` is an expression holding the default value for this flag
//     (which must be implicitly convertible to `T`),
//   * `help` is the help text, which can also be an expression.
//
// This macro expands to a flag named 'FLAGS_name' of type 'T':
//
//   abel::abel_flag<T> FLAGS_name = ...;
//
// Note that all such instances are created as global variables.
//
// For `ABEL_FLAG()` values that you wish to expose to other translation units,
// it is recommended to define those flags within the `.cc` file associated with
// the header where the flag is declared.
//
// Note: do not construct objects of type `abel::abel_flag<T>` directly. Only use the
// `ABEL_FLAG()` macro for such construction.
#define ABEL_FLAG(Type, name, default_value, help) \
  ABEL_FLAG_IMPL(Type, name, default_value, help)

// ABEL_FLAG().on_update()
//
// Defines a flag of type `T` with a callback attached:
//
//   ABEL_FLAG(T, name, default_value, help).on_update(callback);
//
// After any setting of the flag value, the callback will be called at least
// once. A rapid sequence of changes may be merged together into the same
// callback. No concurrent calls to the callback will be made for the same
// flag. Callbacks are allowed to read the current value of the flag but must
// not mutate that flag.
//
// The update mechanism guarantees "eventual consistency"; if the callback
// derives an auxiliary data structure from the flag value, it is guaranteed
// that eventually the flag value and the derived data structure will be
// consistent.
//
// Note: ABEL_FLAG.on_update() does not have a public definition. Hence, this
// comment serves as its API documentation.


// -----------------------------------------------------------------------------
// Implementation details below this section
// -----------------------------------------------------------------------------

// ABEL_FLAG_IMPL macro definition conditional on ABEL_FLAGS_STRIP_NAMES

#if ABEL_FLAGS_STRIP_NAMES
#define ABEL_FLAG_IMPL_FLAGNAME(txt) ""
#define ABEL_FLAG_IMPL_FILENAME() ""
#if !defined(_MSC_VER) || defined(__clang__)
#define ABEL_FLAG_IMPL_REGISTRAR(T, flag) \
  abel::flags_internal::flag_registrar<T, false>(&flag)
#else
#define ABEL_FLAG_IMPL_REGISTRAR(T, flag) \
  abel::flags_internal::flag_registrar<T, false>(flag.GetImpl())
#endif
#else
#define ABEL_FLAG_IMPL_FLAGNAME(txt) txt
#define ABEL_FLAG_IMPL_FILENAME() __FILE__
#if !defined(_MSC_VER) || defined(__clang__)
#define ABEL_FLAG_IMPL_REGISTRAR(T, flag) \
  abel::flags_internal::flag_registrar<T, true>(&flag)
#else
#define ABEL_FLAG_IMPL_REGISTRAR(T, flag) \
  abel::flags_internal::flag_registrar<T, true>(flag.GetImpl())
#endif
#endif

// ABEL_FLAG_IMPL macro definition conditional on ABEL_FLAGS_STRIP_HELP

#if ABEL_FLAGS_STRIP_HELP
#define ABEL_FLAG_IMPL_FLAGHELP(txt) abel::flags_internal::kStrippedFlagHelp
#else
#define ABEL_FLAG_IMPL_FLAGHELP(txt) txt
#endif

// abel_flag_help_gen_for_##name is used to encapsulate both immediate (method Const)
// and lazy (method NonConst) evaluation of help message expression. We choose
// between the two via the call to help_arg in abel::abel_flag instantiation below.
// If help message expression is constexpr evaluable compiler will optimize
// away this whole struct.
#define ABEL_FLAG_IMPL_DECLARE_HELP_WRAPPER(name, txt)                     \
  struct abel_flag_help_gen_for_##name {                                        \
    template <typename T = void>                                           \
    static constexpr const char* Const() {                                 \
      return abel::flags_internal::help_constexpr_wrap(                      \
          ABEL_FLAG_IMPL_FLAGHELP(txt));                                   \
    }                                                                      \
    static std::string NonConst() { return ABEL_FLAG_IMPL_FLAGHELP(txt); } \
  }

#define ABEL_FLAG_IMPL_DECLARE_DEF_VAL_WRAPPER(name, Type, default_value)   \
  static void* abel_flags_init_flag_##name() {                                  \
    return abel::flags_internal::make_from_default_value<Type>(default_value); \
  }

// ABEL_FLAG_IMPL
//
// Note: Name of registrar object is not arbitrary. It is used to "grab"
// global name for FLAGS_no<flag_name> symbol, thus preventing the possibility
// of defining two flags with names foo and nofoo.
#if !defined(_MSC_VER) || defined(__clang__)
#define ABEL_FLAG_IMPL(Type, name, default_value, help)             \
  namespace abel /* block flags in namespaces */ {}                 \
  ABEL_FLAG_IMPL_DECLARE_DEF_VAL_WRAPPER(name, Type, default_value) \
  ABEL_FLAG_IMPL_DECLARE_HELP_WRAPPER(name, help);                  \
  ABEL_CONST_INIT abel::abel_flag<Type> FLAGS_##name{                    \
      ABEL_FLAG_IMPL_FLAGNAME(#name), ABEL_FLAG_IMPL_FILENAME(),    \
      &abel::flags_internal::flag_marshalling_ops<Type>,              \
      abel::flags_internal::help_arg<abel_flag_help_gen_for_##name>(0),   \
      &abel_flags_init_flag_##name};                                    \
  extern bool FLAGS_no##name;                                       \
  bool FLAGS_no##name = ABEL_FLAG_IMPL_REGISTRAR(Type, FLAGS_##name)
#else
// MSVC version uses aggregate initialization. We also do not try to
// optimize away help wrapper.
#define ABEL_FLAG_IMPL(Type, name, default_value, help)               \
  namespace abel /* block flags in namespaces */ {}                   \
  ABEL_FLAG_IMPL_DECLARE_DEF_VAL_WRAPPER(name, Type, default_value)   \
  ABEL_FLAG_IMPL_DECLARE_HELP_WRAPPER(name, help);                    \
  ABEL_CONST_INIT abel::abel_flag<Type> FLAGS_##name{                      \
      ABEL_FLAG_IMPL_FLAGNAME(#name), ABEL_FLAG_IMPL_FILENAME(),      \
      &abel::flags_internal::flag_marshalling_ops<Type>,                \
      &abel_flag_help_gen_for_##name::NonConst, &abel_flags_init_flag_##name}; \
  extern bool FLAGS_no##name;                                         \
  bool FLAGS_no##name = ABEL_FLAG_IMPL_REGISTRAR(Type, FLAGS_##name)
#endif

// ABEL_RETIRED_FLAG
//
// Designates the flag (which is usually pre-existing) as "retired." A retired
// flag is a flag that is now unused by the program, but may still be passed on
// the command line, usually by production scripts. A retired flag is ignored
// and code can't access it at runtime.
//
// This macro registers a retired flag with given name and type, with a name
// identical to the name of the original flag you are retiring. The retired
// flag's type can change over time, so that you can retire code to support a
// custom flag type.
//
// This macro has the same signature as `ABEL_FLAG`. To retire a flag, simply
// replace an `ABEL_FLAG` definition with `ABEL_RETIRED_FLAG`, leaving the
// arguments unchanged (unless of course you actually want to retire the flag
// type at this time as well).
//
// `default_value` is only used as a double check on the type. `explanation` is
// unused.
// TODO(rogeeff): Return an anonymous struct instead of bool, and place it into
// the unnamed namespace.
#define ABEL_RETIRED_FLAG(type, flagname, default_value, explanation) \
  ABEL_ATTRIBUTE_UNUSED static const bool ignored_##flagname =        \
      ([] { return type(default_value); },                            \
       abel::flags_internal::retired_flag<type>(#flagname))

#endif  // ABEL_CONFIG_FLAG_H_
