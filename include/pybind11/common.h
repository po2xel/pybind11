/*
    pybind11/common.h -- Basic macros

    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#if !defined(NAMESPACE_BEGIN)
#  define NAMESPACE_BEGIN(name) namespace name {
#endif
#if !defined(NAMESPACE_END)
#  define NAMESPACE_END(name) }
#endif

#if !defined(PYBIND11_EXPORT)
#  if defined(WIN32) || defined(_WIN32)
#    define PYBIND11_EXPORT __declspec(dllexport)
#  else
#    define PYBIND11_EXPORT __attribute__ ((visibility("default")))
#  endif
#endif

#if defined(_MSC_VER)
#  define PYBIND11_NOINLINE __declspec(noinline)
#else
#  define PYBIND11_NOINLINE __attribute__ ((noinline))
#endif

#define PYBIND11_VERSION_MAJOR 1
#define PYBIND11_VERSION_MINOR 9
#define PYBIND11_VERSION_PATCH dev0

/// Include Python header, disable linking to pythonX_d.lib on Windows in debug mode
#if defined(_MSC_VER)
#  define HAVE_ROUND
#  pragma warning(push)
#  pragma warning(disable: 4510 4610 4512 4005)
#  if _DEBUG
#    define PYBIND11_DEBUG_MARKER
#    undef _DEBUG
#  endif
#endif

#include <Python.h>
#include <frameobject.h>
#include <pythread.h>

#if defined(_WIN32) && (defined(min) || defined(max))
#  error Macro clash with min and max -- define NOMINMAX when compiling your program on Windows
#endif

#if defined(isalnum)
#  undef isalnum
#  undef isalpha
#  undef islower
#  undef isspace
#  undef isupper
#  undef tolower
#  undef toupper
#endif

#if defined(_MSC_VER)
#  if defined(PYBIND11_DEBUG_MARKER)
#    define _DEBUG
#    undef PYBIND11_DEBUG_MARKER
#  endif
#  pragma warning(pop)
#endif

#include <forward_list>
#include <vector>
#include <string>
#include <stdexcept>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <typeindex>

#if PY_MAJOR_VERSION >= 3 /// Compatibility macros for various Python versions
#define PYBIND11_INSTANCE_METHOD_NEW(ptr, class_) PyInstanceMethod_New(ptr)
#define PYBIND11_BYTES_CHECK PyBytes_Check
#define PYBIND11_BYTES_FROM_STRING PyBytes_FromString
#define PYBIND11_BYTES_FROM_STRING_AND_SIZE PyBytes_FromStringAndSize
#define PYBIND11_BYTES_AS_STRING_AND_SIZE PyBytes_AsStringAndSize
#define PYBIND11_BYTES_AS_STRING PyBytes_AsString
#define PYBIND11_BYTES_CHECK PyBytes_Check
#define PYBIND11_LONG_CHECK(o) PyLong_Check(o)
#define PYBIND11_LONG_AS_LONGLONG(o) PyLong_AsLongLong(o)
#define PYBIND11_LONG_AS_UNSIGNED_LONGLONG(o) PyLong_AsUnsignedLongLong(o)
#define PYBIND11_BYTES_NAME "bytes"
#define PYBIND11_STRING_NAME "str"
#define PYBIND11_SLICE_OBJECT PyObject
#define PYBIND11_FROM_STRING PyUnicode_FromString
#define PYBIND11_STR_TYPE ::pybind11::str
#define PYBIND11_OB_TYPE(ht_type) (ht_type).ob_base.ob_base.ob_type
#define PYBIND11_PLUGIN_IMPL(name) \
    extern "C" PYBIND11_EXPORT PyObject *PyInit_##name()
#else
#define PYBIND11_INSTANCE_METHOD_NEW(ptr, class_) PyMethod_New(ptr, nullptr, class_)
#define PYBIND11_BYTES_CHECK PyString_Check
#define PYBIND11_BYTES_FROM_STRING PyString_FromString
#define PYBIND11_BYTES_FROM_STRING_AND_SIZE PyString_FromStringAndSize
#define PYBIND11_BYTES_AS_STRING_AND_SIZE PyString_AsStringAndSize
#define PYBIND11_BYTES_AS_STRING PyString_AsString
#define PYBIND11_BYTES_CHECK PyString_Check
#define PYBIND11_LONG_CHECK(o) (PyInt_Check(o) || PyLong_Check(o))
#define PYBIND11_LONG_AS_LONGLONG(o) (PyInt_Check(o) ? (long long) PyLong_AsLong(o) : PyLong_AsLongLong(o))
#define PYBIND11_LONG_AS_UNSIGNED_LONGLONG(o) (PyInt_Check(o) ? (unsigned long long) PyLong_AsUnsignedLong(o) : PyLong_AsUnsignedLongLong(o))
#define PYBIND11_BYTES_NAME "str"
#define PYBIND11_STRING_NAME "unicode"
#define PYBIND11_SLICE_OBJECT PySliceObject
#define PYBIND11_FROM_STRING PyString_FromString
#define PYBIND11_STR_TYPE ::pybind11::bytes
#define PYBIND11_OB_TYPE(ht_type) (ht_type).ob_type
#define PYBIND11_PLUGIN_IMPL(name) \
    extern "C" PYBIND11_EXPORT PyObject *init##name()
#endif

#if PY_VERSION_HEX >= 0x03050000 && PY_VERSION_HEX < 0x03050200
extern "C" {
    struct _Py_atomic_address { void *value; };
    PyAPI_DATA(_Py_atomic_address) _PyThreadState_Current;
}
#endif

#define PYBIND11_TRY_NEXT_OVERLOAD ((PyObject *) 1) // special failure return code
#define PYBIND11_STRINGIFY(x) #x
#define PYBIND11_TOSTRING(x) PYBIND11_STRINGIFY(x)
#define PYBIND11_INTERNALS_ID "__pybind11_" \
    PYBIND11_TOSTRING(PYBIND11_VERSION_MAJOR) "_" PYBIND11_TOSTRING(PYBIND11_VERSION_MINOR) "__"

#define PYBIND11_PLUGIN(name) \
    static PyObject *pybind11_init(); \
    PYBIND11_PLUGIN_IMPL(name) { \
        try { \
            return pybind11_init(); \
        } catch (const std::exception &e) { \
            PyErr_SetString(PyExc_ImportError, e.what()); \
            return nullptr; \
        } \
    } \
    PyObject *pybind11_init()

NAMESPACE_BEGIN(pybind11)

typedef Py_ssize_t ssize_t;

/// Approach used to cast a previously unknown C++ instance into a Python object
enum class return_value_policy : uint8_t {
    /** This is the default return value policy, which falls back to the policy
        return_value_policy::take_ownership when the return value is a pointer.
        Otherwise, it uses return_value::move or return_value::copy for rvalue
        and lvalue references, respectively. See below for a description of what
        all of these different policies do. */
    automatic = 0,

    /** As above, but use policy return_value_policy::reference when the return
        value is a pointer. This is the default conversion policy for function
        arguments when calling Python functions manually from C++ code (i.e. via
        handle::operator()). You probably won't need to use this. */
    automatic_reference,

    /** Reference an existing object (i.e. do not create a new copy) and take
        ownership. Python will call the destructor and delete operator when the
        object’s reference count reaches zero. Undefined behavior ensues when
        the C++ side does the same.. */
    take_ownership,

    /** Create a new copy of the returned object, which will be owned by
        Python. This policy is comparably safe because the lifetimes of the two
        instances are decoupled. */
    copy,

    /** Use std::move to move the return value contents into a new instance
        that will be owned by Python. This policy is comparably safe because the
        lifetimes of the two instances (move source and destination) are
        decoupled. */
    move,

    /** Reference an existing object, but do not take ownership. The C++ side
        is responsible for managing the object’s lifetime and deallocating it
        when it is no longer used. Warning: undefined behavior will ensue when
        the C++ side deletes an object that is still referenced and used by
        Python. */
    reference,

    /** This policy only applies to methods and properties. It references the
        object without taking ownership similar to the above
        return_value_policy::reference policy. In contrast to that policy, the
        function or property’s implicit this argument (called the parent) is
        considered to be the the owner of the return value (the child).
        pybind11 then couples the lifetime of the parent to the child via a
        reference relationship that ensures that the parent cannot be garbage
        collected while Python is still using the child. More advanced
        variations of this scheme are also possible using combinations of
        return_value_policy::reference and the keep_alive call policy */
    reference_internal
};

/// Information record describing a Python buffer object
struct buffer_info {
    void *ptr = nullptr;         // Pointer to the underlying storage
    size_t itemsize = 0;         // Size of individual items in bytes
    size_t size = 0;             // Total number of entries
    std::string format;          // For homogeneous buffers, this should be set to format_descriptor<T>::format()
    size_t ndim = 0;             // Number of dimensions
    std::vector<size_t> shape;   // Shape of the tensor (1 entry per dimension)
    std::vector<size_t> strides; // Number of entries between adjacent entries (for each per dimension)

    buffer_info() { }

    buffer_info(void *ptr, size_t itemsize, const std::string &format, size_t ndim,
                const std::vector<size_t> &shape, const std::vector<size_t> &strides)
        : ptr(ptr), itemsize(itemsize), size(1), format(format),
          ndim(ndim), shape(shape), strides(strides) {
        for (size_t i = 0; i < ndim; ++i)
            size *= shape[i];
    }

    buffer_info(void *ptr, size_t itemsize, const std::string &format, size_t size)
    : buffer_info(ptr, itemsize, format, 1, std::vector<size_t> { size },
                  std::vector<size_t> { itemsize }) { }

    buffer_info(Py_buffer *view, bool ownview = true)
        : ptr(view->buf), itemsize((size_t) view->itemsize), size(1), format(view->format),
          ndim((size_t) view->ndim), shape((size_t) view->ndim), strides((size_t) view->ndim), view(view), ownview(ownview) {
        for (size_t i = 0; i < (size_t) view->ndim; ++i) {
            shape[i] = (size_t) view->shape[i];
            strides[i] = (size_t) view->strides[i];
            size *= shape[i];
        }
    }

    buffer_info(const buffer_info &) = delete;
    buffer_info& operator=(const buffer_info &) = delete;

    buffer_info(buffer_info &&other) {
        (*this) = std::move(other);
    }

    buffer_info& operator=(buffer_info &&rhs) {
        ptr = rhs.ptr;
        itemsize = rhs.itemsize;
        size = rhs.size;
        format = std::move(rhs.format);
        ndim = rhs.ndim;
        shape = std::move(rhs.shape);
        strides = std::move(rhs.strides);
        std::swap(view, rhs.view);
        std::swap(ownview, rhs.ownview);
        return *this;
    }

    ~buffer_info() {
        if (view && ownview) { PyBuffer_Release(view); delete view; }
    }

private:
    Py_buffer *view = nullptr;
    bool ownview = false;
};

NAMESPACE_BEGIN(detail)

inline static constexpr int log2(size_t n, int k = 0) { return (n <= 1) ? k : log2(n >> 1, k + 1); }

inline std::string error_string();

/// Core part of the 'instance' type which POD (needed to be able to use 'offsetof')
template <typename type> struct instance_essentials {
    PyObject_HEAD
    type *value;
    PyObject *weakrefs;
    bool owned : 1;
    bool constructed : 1;
};

/// PyObject wrapper around generic types, includes a special holder type that is responsible for lifetime management
template <typename type, typename holder_type = std::unique_ptr<type>> struct instance : instance_essentials<type> {
    holder_type holder;
};

struct overload_hash {
    inline std::size_t operator()(const std::pair<const PyObject *, const char *>& v) const {
        size_t value = std::hash<const void *>()(v.first);
        value ^= std::hash<const void *>()(v.second)  + 0x9e3779b9 + (value<<6) + (value>>2);
        return value;
    }
};

/// Internal data struture used to track registered instances and types
struct internals {
    std::unordered_map<std::type_index, void*> registered_types_cpp;   // std::type_index -> type_info
    std::unordered_map<const void *, void*> registered_types_py;       // PyTypeObject* -> type_info
    std::unordered_multimap<const void *, void*> registered_instances; // void * -> PyObject*
    std::unordered_set<std::pair<const PyObject *, const char *>, overload_hash> inactive_overload_cache;
    std::forward_list<void (*) (std::exception_ptr)> registered_exception_translators;
#if defined(WITH_THREAD)
    decltype(PyThread_create_key()) tstate = 0; // Usually an int but a long on Cygwin64 with Python 3.x
    PyInterpreterState *istate = nullptr;
#endif
};

/// Return a reference to the current 'internals' information
inline internals &get_internals();

/// Index sequence for convenient template metaprogramming involving tuples
template<size_t ...> struct index_sequence  { };
template<size_t N, size_t ...S> struct make_index_sequence : make_index_sequence <N - 1, N - 1, S...> { };
template<size_t ...S> struct make_index_sequence <0, S...> { typedef index_sequence<S...> type; };

/// Strip the class from a method type
template <typename T> struct remove_class { };
template <typename C, typename R, typename... A> struct remove_class<R (C::*)(A...)> { typedef R type(A...); };
template <typename C, typename R, typename... A> struct remove_class<R (C::*)(A...) const> { typedef R type(A...); };

/// Helper template to strip away type modifiers
template <typename T> struct intrinsic_type                       { typedef T type; };
template <typename T> struct intrinsic_type<const T>              { typedef typename intrinsic_type<T>::type type; };
template <typename T> struct intrinsic_type<T*>                   { typedef typename intrinsic_type<T>::type type; };
template <typename T> struct intrinsic_type<T&>                   { typedef typename intrinsic_type<T>::type type; };
template <typename T> struct intrinsic_type<T&&>                  { typedef typename intrinsic_type<T>::type type; };
template <typename T, size_t N> struct intrinsic_type<const T[N]> { typedef typename intrinsic_type<T>::type type; };
template <typename T, size_t N> struct intrinsic_type<T[N]>       { typedef typename intrinsic_type<T>::type type; };
template <typename T> using intrinsic_t = typename intrinsic_type<T>::type;

/// Helper type to replace 'void' in some expressions
struct void_type { };

/// from __cpp_future__ import (convenient aliases from C++14/17)
template <bool B> using bool_constant = std::integral_constant<bool, B>;
template <class T> using negation = bool_constant<!T::value>;
template <bool B, typename T = void> using enable_if_t = typename std::enable_if<B, T>::type;
template <bool B, typename T, typename F> using conditional_t = typename std::conditional<B, T, F>::type;

/// Compile-time integer sum
constexpr size_t constexpr_sum() { return 0; }
template <typename T, typename... Ts>
constexpr size_t constexpr_sum(T n, Ts... ns) { return size_t{n} + constexpr_sum(ns...); }

/// Return true if all/any Ts satify Predicate<T>
#if !defined(_MSC_VER)
template <template<typename> class Predicate, typename... Ts>
using all_of_t = bool_constant<(constexpr_sum(Predicate<Ts>::value...) == sizeof...(Ts))>;
template <template<typename> class Predicate, typename... Ts>
using any_of_t = bool_constant<(constexpr_sum(Predicate<Ts>::value...) > 0)>;
#else
// MSVC workaround (2015 Update 3 has issues with some member type aliases and constexpr)
template <template<typename> class P, typename...> struct all_of_t : std::true_type { };
template <template<typename> class P, typename T, typename... Ts>
struct all_of_t<P, T, Ts...> : conditional_t<P<T>::value, all_of_t<P, Ts...>, std::false_type> { };
template <template<typename> class P, typename...> struct any_of_t : std::false_type { };
template <template<typename> class P, typename T, typename... Ts>
struct any_of_t<P, T, Ts...> : conditional_t<P<T>::value, std::true_type, any_of_t<P, Ts...>> { };
#endif

// Extracts the first type from the template parameter pack matching the predicate, or Default if none match.
template <template<class> class Predicate, class Default, class... Ts> struct first_of;
template <template<class> class Predicate, class Default> struct first_of<Predicate, Default> {
    using type = Default;
};
template <template<class> class Predicate, class Default, class T, class... Ts>
struct first_of<Predicate, Default, T, Ts...> {
    using type = typename std::conditional<
        Predicate<T>::value,
        T,
        typename first_of<Predicate, Default, Ts...>::type
    >::type;
};
template <template<class> class Predicate, class Default, class... T> using first_of_t = typename first_of<Predicate, Default, T...>::type;

// Counts the number of types in the template parameter pack matching the predicate
template <template<typename> class Predicate, typename... Ts> struct count_t;
template <template<typename> class Predicate> struct count_t<Predicate> : std::integral_constant<size_t, 0> {};
template <template<typename> class Predicate, class T, class... Ts>
struct count_t<Predicate, T, Ts...> : std::integral_constant<size_t,
    Predicate<T>::value + count_t<Predicate, Ts...>::value> {};

/// Defer the evaluation of type T until types Us are instantiated
template <typename T, typename... /*Us*/> struct deferred_type { using type = T; };
template <typename T, typename... Us> using deferred_t = typename deferred_type<T, Us...>::type;

/// Ignore that a variable is unused in compiler warnings
inline void ignore_unused(const int *) { }

NAMESPACE_END(detail)

#define PYBIND11_RUNTIME_EXCEPTION(name) \
    class name : public std::runtime_error { public: \
        name(const std::string &w) : std::runtime_error(w) { }; \
        name(const char *s) : std::runtime_error(s) { }; \
        name() : std::runtime_error("") { } \
    };

// C++ bindings of core Python exceptions
class error_already_set : public std::runtime_error { public: error_already_set() : std::runtime_error(detail::error_string())  {} };
PYBIND11_RUNTIME_EXCEPTION(stop_iteration)
PYBIND11_RUNTIME_EXCEPTION(index_error)
PYBIND11_RUNTIME_EXCEPTION(key_error)
PYBIND11_RUNTIME_EXCEPTION(value_error)
PYBIND11_RUNTIME_EXCEPTION(type_error)
PYBIND11_RUNTIME_EXCEPTION(cast_error) /// Thrown when pybind11::cast or handle::call fail due to a type casting error
PYBIND11_RUNTIME_EXCEPTION(reference_cast_error) /// Used internally

[[noreturn]] PYBIND11_NOINLINE inline void pybind11_fail(const char *reason) { throw std::runtime_error(reason); }
[[noreturn]] PYBIND11_NOINLINE inline void pybind11_fail(const std::string &reason) { throw std::runtime_error(reason); }

/// Format strings for basic number types
#define PYBIND11_DECL_FMT(t, v) template<> struct format_descriptor<t> \
    { static constexpr const char* value = v; /* for backwards compatibility */ \
      static std::string format() { return value; } }

template <typename T, typename SFINAE = void> struct format_descriptor { };

template <typename T> struct format_descriptor<T, typename std::enable_if<std::is_integral<T>::value>::type> {
    static constexpr const char value[2] =
        { "bBhHiIqQ"[detail::log2(sizeof(T))*2 + (std::is_unsigned<T>::value ? 1 : 0)], '\0' };
    static std::string format() { return value; }
};

template <typename T> constexpr const char format_descriptor<
    T, typename std::enable_if<std::is_integral<T>::value>::type>::value[2];

PYBIND11_DECL_FMT(float, "f");
PYBIND11_DECL_FMT(double, "d");
PYBIND11_DECL_FMT(bool, "?");

/// Dummy destructor wrapper that can be used to expose classes with a private destructor
struct nodelete { template <typename T> void operator()(T*) { } };

NAMESPACE_END(pybind11)
