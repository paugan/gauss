#include <os.h>

#if defined(GAUSS_OS_UNIX)

int gauss_os_mutex_lock(gauss_mutex_t *lock)
{
    int rc;

    rc = pthread_mutex_lock(lock);

    if (rc != 0)
        return -1;

    return 0;
}

int gauss_os_mutex_trylock(gauss_mutex_t *lock)
{
    int rc;

    rc = pthread_mutex_trylock(lock);

    if (rc != 0)
        return -1;

    return 0;
}

int gauss_os_mutex_unlock(gauss_mutex_t *lock)
{
    int rc;

    rc = pthread_mutex_unlock(lock);

    if (rc != 0)
        return -1;

    return 0;
}

int gauss_os_tls_key_init(gauss_tls_key_t *tls, void (*__tls_dtor)(void *uptr))
{
    int rc;

    rc = pthread_key_create(key, __tls_dtor);

    if (rc != 0)
        return -1;

    return 0;
}

int gauss_os_tls_key_get(gauss_tls_key_t *tls, void **uptr)
{
    *uptr = pthread_getspecific(*tls);

    if (!(*uptr))
        return -1;

    return 0;
}

int gauss_os_tls_key_set(gauss_tls_key_t *tls, const void *vptr)
{
    int rc;

    rc = pthread_setspecific(*tls, vptr);

    if (rc != 0)
        return -1;

    return 0;
}

int gauss_os_tls_key_free(gauss_tls_key_t *tls)
{
    int rc;

    rc = pthread_key_delete(*tls);

    if (rc != 0)
        return -1;

    return 0;
}

void *gauss_os_alloc(size_t size)
{
    void *uptr;

    uptr = malloc(size);

    if (!uptr)
        return NULL;

    return uptr;
}

void *gauss_os_calloc(size_t size)
{
    void *uptr;

    uptr = gauss_os_alloc(size);

    if (!uptr)
        return NULL;

    gauss_os_zero_buf(uptr, size);
    return uptr;
}

void *gauss_os_palloc(size_t size)
{
    void *uptr;

    uptr = mmap(
        NULL,
        size * GAUSS_ALLOC_PAGE_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );

    if (uptr == MAP_FAILED)
        return NULL;

    return uptr;
}

void *gauss_os_pcalloc(size_t size)
{
    void *uptr;

    uptr = gauss_os_palloc(size);

    if (!uptr)
        return NULL;

    gauss_os_zero_buf(uptr, size * GAUSS_ALLOC_PAGE_SIZE);
    return uptr;
}

void *gauss_os_alloc_aligned(size_t size, size_t align)
{
    int ret;
    void *uptr;

    ret = posix_memalign(&uptr, size, align);

    if (ret != 0)
        return NULL;

    return uptr;
}

void *gauss_os_calloc_aligned(size_t size, size_t align)
{
    void *uptr;

    uptr = gauss_os_alloc_aligned(size, align);

    if (!uptr)
        return NULL;

    gauss_os_zero_buf(uptr, gauss_os_alloc_calc_align(size, align));
    return uptr;
}

void gauss_os_free(void *uptr, size_t size)
{
    if (!uptr)
        return;

    GAUSS_UNUSED(size);
    free(uptr);
    return;
}

void gauss_os_pfree(void *uptr, size_t size)
{
    if (!uptr)
        return;

    munmap(uptr, size * GAUSS_ALLOC_PAGE_SIZE);
    return;
}

void gauss_os_free_aligned(void *uptr, size_t size, size_t align)
{
    if (!uptr)
        return;

    GAUSS_UNUSED(size);
    GAUSS_UNUSED(align);
    free(uptr);
    return;
}

uint16_t gauss_os_thread_id(void)
{
    return pthread_self();
}

size_t gauss_os_page_size(void)
{
    return sysconf(_SC_PAGESIZE);
}

size_t gauss_os_cache_line_size(void)
{
    size_t cline;

    cline = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);

    return cline > 0 ? cline : GAUSS_CACHE_LINE_SIZE;
}

uint32_t gauss_os_cpu_count(void)
{
    uint32_t ncpu;

    ncpu = sysconf(_SC_NPROCESSORS_ONLN);

    return ncpu > 0 ? ncpu : 1;
}

/* memory barrier macros as funcs. */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
#include <stdatomic.h>
#define __gauss_os_memory_barrier() atomic_thread_fence(memory_order_seq_cst)
#elif defined(__GNUC__) || defined(__clang__)
#define __gauss_os_memory_barrier() __atomic_thread_fence(__ATOMIC_SEQ_CST)
#elif defined(GAUSS_ARCH_X86_32) || defined(GAUSS_ARCH_X86_64)
static GAUSS_INLINE void ___gauss_os_memory_barrier(void)
{
    __asm__ __volatile__("mfence" ::: "memory");
    return;
}

#define __gauss_os_memory_barrier() ___gauss_os_memory_barrier()
#elif defined(GAUSS_ARCH_ARM64)
static GAUSS_INLINE void ___gauss_os_memory_barrier(void)
{
    __asm__ __volatile__("dmb ish" ::: "memory");
    return;
}

#define __gauss_os_memory_barrier() ___gauss_os_memory_barrier()
#elif defined(GAUSS_ARCH_PPC32) || defined(GAUSS_ARCH_PPC64)
static GAUSS_INLINE void ___gauss_os_memory_barrier(void)
{
    __asm__ __volatile__("sync" ::: "memory");
    return;
}

#define __gauss_os_memory_barrier() ___gauss_os_memory_barrier()
#endif

void gauss_os_memory_barrier(void)
{
    __gauss_os_memory_barrier();
    return;
}

void gauss_os_sleep(uint64_t msecs)
{
    usleep(msecs * 1000);
    return;
}

void *gauss_os_zero_buf(void *ubuf, size_t size)
{
    return memset(ubuf, 0, size);
}

char *gauss_os_strdup(const char *ubuf)
{
    return strdup(ubuf);
}

#endif
