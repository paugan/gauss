#ifndef __GAUSS_OS_H
#define __GAUSS_OS_H

/* arch detection */
#if defined(__x86_64__) || defined(_M_X64)
#define GAUSS_ARCH_X86_64 1
#elif defined(__i386__) || defined(_M_IX86)
#define GAUSS_ARCH_X86_32 1
#elif defined(__aarch64__) || defined(_M_ARM64)
#define GAUSS_ARCH_ARM64 1
#elif defined(__arm__) || defined(_M_ARM)
#define GAUSS_ARCH_ARM32 1
#elif defined(__powerpc64__) || defined(__ppc64__)
#define GAUSS_ARCH_PPC64 1
#elif defined(__powerpc__) || defined(__ppc__)
#define GAUSS_ARCH_PPC32 1
#elif defined(__riscv) && __riscv_xlen == 64
#define GAUSS_ARCH_RISCV64 1
#endif

/* data model detection */
#if defined(__LP64__) || defined(_LP64)
#define GAUSS_DATA_MODEL_64 1
#else
#define GAUSS_DATA_MODEL_64 0
#endif

/* OS detection */
#if defined(_WIN32) || defined(_WIN64)
#define GAUSS_OS_WIN 1
#define GAUSS_OS_UNIX 0
#elif defined(__linux__)
#define GAUSS_OS_LINUX 1
#define GAUSS_OS_UNIX 1
#elif defined(__APPLE__) && defined(__MACH__)
#define GAUSS_OS_MACOS 1
#define GAUSS_OS_UNIX 1
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#define GAUSS_OS_BSD 1
#define GAUSS_OS_UNIX 1
#elif defined(__unix__) || defined(__unix) || defined(unix)
#define GAUSS_OS_UNIX 1
#define GAUSS_OS_OTHER_UNIX 1
#else
#error "Unknown platform"
#endif

/* platform-bits detection */
#if defined(_WIN32) || defined(_WIN64)
#if defined(_WIN64)
#define GAUSS_BITS_64 1
#elif defined(_WIN32)
#define GAUSS_BITS_64 0
#endif
#elif defined(GAUSS_OS_UNIX) && defined(GAUSS_OS_LINUX)
#if defined(GAUSS_ARCH_X86_64) || defined(GAUSS_ARCH_ARM64) || \
    defined(GAUSS_ARCH_PPC64) || defined(GAUSS_DATA_MODEL_64)
#define GAUSS_BITS_64 1
#else
#define GAUSS_BITS_64 0
#endif
#elif defined(GAUSS_OS_UNIX) && defined(GAUSS_OS_MACOS)
/*
 * Darwin kernel: __LP64__ is set when long + pointers are 64-bit.
 * Also covers Apple Silicon (__aarch64__) and Intel (__x86_64__).
 */
#if defined(GAUSS_DATA_MODEL_64) || defined(GAUSS_ARCH_X86_64) || defined(GAUSS_ARCH_ARM64)
#define GAUSS_BITS_64 1
#else
#define GAUSS_BITS_64 0
#endif
#elif defined(GAUSS_OS_UNIX) && (defined(GAUSS_OS_BSD) || defined(GAUSS_OS_OTHER_UNIX))
#if defined(GAUSS_DATA_MODEL_64) || defined(GAUSS_ARCH_X86_64) || defined(GAUSS_ARCH_ARM64)
#define GAUSS_BITS_64 1
#else
#define GAUSS_BITS_64 0
#endif
#else
/*
 * Fallback for anything not explicitly handled above: ask the
 * compiler directly instead of guessing per-OS. This works on
 * virtually any GCC/Clang-family compiler, and the UINTPTR_MAX
 * branch works anywhere (available in header file: <stdint.h>)
 * is available.
 */
#if defined(__SIZEOF_POINTER__)
#if __SIZEOF_POINTER__ == 0
#define GAUSS_BITS_64 1
#else
#define GAUSS_BITS_64 0
#endif
#elif defined(UINTPTR_MAX) && UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFULL
#define GAUSS_BITS_64 1
#else
#define GAUSS_BITS_64 0
#endif
#endif

/* detect compiler */
#if defined(__GNUC__) || defined(__clang__)
#define GAUSS_GCC_OR_CLANG 1
#define GAUSS_COMPILER "GCC/Clang"
#elif defined(_MSC_VER)
#define GAUSS_MSC 1
#define GAUSS_COMPILER "MSVC"
#else
#define GAUSS_COMPILER "Unknown"
#endif

/* check for fast-user mutex support */
#if defined(GAUSS_OS_LINUX) && defined(__linux__)
#define GAUSS_HAVE_FUTEX 1
#endif

/* check for large file support */
#if defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS == 64
#define GAUSS_HAVE_LARGE_FILE 1
#endif

/* check for compiler attributes availability */
#if defined(GAUSS_GCC_OR_CLANG)
#define GAUSS_PURE __attribute__((pure))
#define GAUSS_NORETURN __attribute__((noreturn))
#define GAUSS_DEPRECATED __attribute__((deprecated))
#define GAUSS_PACKED __attribute__((packed))
#define GAUSS_ALIGNED(x) __attribute__((aligned((x))))
#define GAUSS_MALLOC __attribute__((malloc))
#define GAUSS_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#define GAUSS_COLD __attribute__((cold))
#define GAUSS_HOT __attribute__((hot))
#elif defined(GAUSS_MSC)
#define GAUSS_PURE
#define GAUSS_NORETURN __declspec(noreturn)
#define GAUSS_DEPRECATED __declspec(deprecated)
#define GAUSS_PACKED
#define GAUSS_ALIGNED(x) __declspec(align((x)))
#define GAUSS_MALLOC
#define GAUSS_WARN_UNUSED_RESULT
#define GAUSS_COLD
#define GAUSS_HOT
#else
#define GAUSS_PURE
#define GAUSS_NORETURN
#define GAUSS_DEPRECATED
#define GAUSS_PACKED
#define GAUSS_ALIGNED(x)
#define GAUSS_MALLOC
#define GAUSS_WARN_UNUSED_RESULT
#define GAUSS_COLD
#define GAUSS_HOT
#endif

/* check for function inlining support */
#if defined(GAUSS_GCC_OR_CLANG)
#define GAUSS_INLINE inline __attribute__((always_inline))
#elif defined(GAUSS_MSC)
#define GAUSS_INLINE __forceinline
#else
#define GAUSS_INLINE inline
#endif

/* check for restrict keyword support */
#if defined(GAUSS_GCC_OR_CLANG) || defined(GAUSS_MSC)
#define GAUSS_RESTRICT __restrict
#else
#define GAUSS_RESTRICT
#endif

/* check for thread-local storage support */
#if defined(GAUSS_OS_WIN)
#define GAUSS_TLS __declspec(thread)
#elif defined(OHM_GCC_OR_CLANG)
#define GAUSS_TLS __thread
#else
#define GAUSS_TLS static
#endif

/* export/import macros, supplied from compile-time flags */
#if defined(GAUSS_BUILD_SHARED) && defined(GAUSS_OS_WIN)
#define GAUSS_EXPORT __declspec(dllexport)
#define GAUSS_IMPORT __declspec(dllimport)
#elif defined(GAUSS_BUILD_SHARED) && defined(GAUSS_GCC_OR_CLANG)
#define GAUSS_EXPORT __attribute__((visibility("default")))
#define GAUSS_IMPORT
#else
#define GAUSS_EXPORT
#define GAUSS_IMPORT
#endif

/* cache line size */
#ifndef GAUSS_CACHE_LINE_SIZE
#define GAUSS_CACHE_LINE_SIZE 64
#endif

#define GAUSS_CACHE_ALIGN GAUSS_ALIGNED(GAUSS_CACHE_LINE_SIZE)

/* endianness */
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define GAUSS_BIG_ENDIAN 1
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define GAUSS_LITTLE_ENDIAN 1
#elif defined(_BYTE_ORDER) && _BYTE_ORDER == _BIG_ENDIAN
#define GAUSS_BIG_ENDIAN 1
#elif defined(_BYTE_ORDER) && _BYTE_ORDER == _LITTLE_ENDIAN
#define GAUSS_LITTLE_ENDIAN 1
#elif defined(__BIG_ENDIAN__)
#define GAUSS_BIG_ENDIAN 1
#elif defined(__LITTLE_ENDIAN__)
#define GAUSS_LITTLE_ENDIAN 1
#else
#define GAUSS_LITTLE_ENDIAN 1
#endif

/* branch prediction */
#if defined(GAUSS_GCC_OR_CLANG)
#define GAUSS_LIKELY(x) __builtin_expect((x), 1)
#define GAUSS_UNLIKELY(x) __builtin_expect((x), 0)
#else
#define GAUSS_LIKELY(x)
#define GAUSS_UNLIKELY(x)
#endif

#if defined(GAUSS_OS_WIN)
#include <windows.h>
#elif defined(GAUSS_OS_UNIX) && defined(GAUSS_OS_LINUX) && GAUSS_OS_LINUX == 1
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
// and maybe more of them..
#endif

/* mutex types */
#if defined(GAUSS_OS_WIN)
typedef CRITICAL_SECTION gauss_mutex_t;
#elif defined(GAUSS_OS_UNIX) && defined(GAUSS_OS_LINUX) && GAUSS_OS_LINUX == 1
typedef pthread_mutex_t gauss_mutex_t;
#endif

/* platform-dependent mutex static initialization value */
#if defined(GAUSS_OS_WIN)
#define GAUSS_MLOCK_INIT ({0})
#elif defined(GAUSS_OS_UNIX) && defined(GAUSS_OS_LINUX) && GAUSS_OS_LINUX == 1
#define GAUSS_MLOCK_INIT PTHREAD_MUTEX_INITIALIZER
#endif

static GAUSS_INLINE int gauss_os_mutex_init(gauss_mutex_t *lock)
{
    *lock = GAUSS_MLOCK_INIT;
    return 0;
}

int gauss_os_mutex_lock(gauss_mutex_t *lock);
int gauss_os_mutex_trylock(gauss_mutex_t *lock);
int gauss_os_mutex_unlock(gauss_mutex_t *lock);

/* atomic macros and macros as funcs */
#if defined(GAUSS_OS_WIN)
#define gauss_atomic_inc(x) InterlockedIncrement((LONG *)(x))
#define gauss_atomic_dec(x) InterlockedDecrement((LONG *)(x))
#define gauss_atomic_add(x, a) InterlockedAdd((LONG *)(x), (a))
#define gauss_atomic_sub(x, a) InterlockedAdd((LONG *)(x), -(a))
#define gauss_atomic_load(x) InterlockedOr((LONG *)(x), 0)
#define gauss_atomic_fetch_and_add(x, a) InterlockedExchangeAdd((LONG *)(x), (a))
#define gauss_atomic_cmpxchg(x, o, n) InterlockedCompareExchange((LONG *)(x), (n), (o)) == (o)
#elif defined(GAUSS_OS_UNIX) && !defined(__ATOMIC_RELAXED)
#define gauss_atomic_inc(x) atomic_fetch_add((x), 1)
#define gauss_atomic_dec(x) atomic_fetch_add((x), -1)
#define gauss_atomic_add(x, a) atomic_fetch_add((x), (a))
#define gauss_atomic_sub(x, a) atomic_fetch_add((x), -(a))
#define gauss_atomic_load(x) atomic_load((x))
#define gauss_atomic_fetch_and_add(x, a) atomic_fetch_add((x), (a))
#define gauss_atomic_cmpxchg(x, o, n) do { \
    atomic_compare_exchange_strong((x), &(o), (n)); \
} while (0)
#elif defined(GAUSS_OS_UNIX) && defined(__ATOMIC_RELAXED)
#define gauss_atomic_inc(x) __atomic_fetch_add((x), 1, __ATOMIC_RELAXED)
#define gauss_atomic_dec(x) __atomic_fetch_sub((x), 1, __ATOMIC_RELAXED)
#define gauss_atomic_add(x, a) __atomic_fetch_add((x), (a), __ATOMIC_RELAXED)
#define gauss_atomic_sub(x, a) __atomic_fetch_sub((x), (a), __ATOMIC_RELAXED)
#define gauss_atomic_load(x) __atomic_load_n((x), __ATOMIC_RELAXED)
#define gauss_atomic_fetch_and_add(x, a) __atomic_fetch_add((x), (a), __ATOMIC_RELAXED)
#define gauss_atomic_cmpxchg(x, o, n) do { \
    __atomic_compare_exchange_n((x), &(o), (n), 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED); \
} while (0)
#else
static GAUSS_INLINE int ___gauss_atomic_load(volatile int *x)
{
    int rc, ret;
    gauss_mutex_t lock;

    gauss_os_mutex_init(&lock);

    rc = gauss_os_mutex_lock(&lock);

    if (rc < 0) {
        ret = -1;
        goto __fallback;
    }

    ret = *x;
    rc  = gauss_os_mutex_unlock(&lock);

    if (rc < 0)
        ret = -1;

__fallback:
    return ret;
}

static GAUSS_INLINE int ___gauss_atomic_fetch_and_add(volatile int *x, int v)
{
    int rc, ret;
    gauss_mutex_t lock;

    gauss_os_mutex_init(&lock);

    rc = gauss_os_mutex_lock(&lock);

    if (rc < 0) {
        ret = -1;
        goto __fallback;
    }

    ret = *x;
    *x += v;

    rc = gauss_os_mutex_unlock(&lock);

    if (rc < 0)
        ret = -1;

__fallback:
    return ret;
}

static GAUSS_INLINE int ___gauss_atomic_cmpxchg(volatile int *x, int old, int new)
{
    int rc, res;
    gauss_mutex_t lock;

    gauss_os_mutex_init(&lock);

    rc = gauss_os_mutex_lock(&lock);

    if (rc < 0) {
        ret = -1;
        goto __fallback;
    }

    if (*x == old)
        *x = new;

    rc = gauss_os_mutex_unlock(&lock);

    if (rc < 0) {
        ret = -1;
        goto __fallback;
    }

    ret = *x == old;

__fallback:
    return ret;
}

#define gauss_atomic_inc(x) ___gauss_atomic_fetch_and_add((x), 1)
#define gauss_atomic_dec(x) ___gauss_atomic_fetch_and_add((x), -1)
#define gauss_atomic_add(x, a) ___gauss_atomic_fetch_and_add((x), (a))
#define gauss_atomic_sub(x, a) ___gauss_atomic_fetch_and_add((x), -(a))
#define gauss_atomic_load(x) ___gauss_atomic_load((x))
#define gauss_atomic_fetch_and_add(x, a) ___gauss_atomic_fetch_and_add((x), (a))
#define gauss_atomic_cmpxchg(x, o, n) ___gauss_atomic_cmpxchg((x), (o), (n))
#endif

#define GAUSS_UNUSED(x) ((void)(x))

#if defined(GAUSS_OS_WIN)
typedef DWORD gauss_tls_key_t;
#elif defined(GAUSS_OS_UNIX) && defined(GAUSS_OS_LINUX) && GAUSS_OS_LINUX == 1
typedef pthread_key_t gauss_tls_key_t;
#endif

/* thread-local storage proto funcs. */
int gauss_os_tls_key_init(gauss_tls_key_t *tls, void (*__tls_dtor)(void *uptr));
int gauss_os_tls_key_get(gauss_tls_key_t *tls, void **uptr);
int gauss_os_tls_key_set(gauss_tls_key_t *tls, const void *vptr);
int gauss_os_tls_key_free(gauss_tls_key_t *tls);

/* platform-native memory allocator proto/inline funcs. */
static GAUSS_INLINE size_t gauss_os_alloc_calc_align(size_t size, size_t align)
{
    return size + align - 1 + sizeof(void *);
}

void *gauss_os_alloc(size_t size);
void *gauss_os_calloc(size_t size);
void *gauss_os_palloc(size_t size);
void *gauss_os_pcalloc(size_t size);
void *gauss_os_alloc_aligned(size_t size, size_t align);
void *gauss_os_calloc_aligned(size_t size, size_t align);

void gauss_os_free(void *uptr, size_t size);
void gauss_os_pfree(void *uptr, size_t size);
void gauss_os_free_aligned(void *uptr, size_t size, size_t align);

/* thread specific proto funcs. */
uint16_t gauss_os_thread_id(void);

/* proc. native proto funcs. */
size_t gauss_os_page_size(void);
size_t gauss_os_cache_line_size(void);
uint32_t gauss_os_cpu_count(void);
void gauss_os_memory_barrier(void);

/* platform time specific proto funcs. */
void gauss_os_sleep(uint64_t msecs);

/* platform-specific buffer proto funcs. */
void *gauss_os_zero_buf(void *ubuf, size_t size);

#endif /* __GAUSS_OS_H */
