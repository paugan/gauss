#include <os.h>

#if defined(GAUSS_OS_WIN)

static HANDLE __gauss_pgheap = NULL;

int gauss_os_mutex_lock(gauss_mutex_t *lock)
{
    EnterCriticalSection(lock);
    return 0;
}

int gauss_os_mutex_trylock(gauss_mutex_t *lock)
{
    BOOL ret;

    ret = TryEnterCriticalSection(lock);

    if (!ret)
        return -1;

    return 0;
}

int gauss_os_mutex_unlock(gauss_mutex_t *lock)
{
    LeaveCriticalSection(lock);
    return 0;
}

int gauss_os_tls_key_init(gauss_tls_key_t *tls, void (*__tls_dtor)(void *uptr))
{
    *tls = FlsAlloc((PFLS_CALLBACK_FUNCTION)__tls_dtor);

    if (*tls == FLS_OUT_OF_INDEXES)
        return -1;

    return 0;
}

int gauss_os_tls_key_get(gauss_tls_key_t *tls, void **uptr)
{
    void *ret;

    ret = FlsGetValue(*tls);

    if (!ret)
        return -1;

    *uptr = ret;
    return 0;
}

int gauss_os_tls_key_set(gauss_tls_key_t *tls, const void *vptr)
{
    BOOL ret;

    ret = FlsSetValue(*tls, vptr);

    if (!ret)
        return -1;

    return 0;
}

int gauss_os_tls_key_free(gauss_tls_key_t *tls)
{
    BOOL ret;

    ret = FlsFree(*tls);

    if (!ret)
        return -1;

    return 0;
}

void *gauss_os_alloc(size_t size)
{
    void *uptr;

    /* singleton like heap initialization */
    if (!__gauss_pgheap)
        __gauss_pgheap = GetProcessHeap();

    uptr = HeapAlloc(__gauss_pgheap, 0, size);

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

    uptr = VirtualAlloc(
        NULL,
        size * GAUSS_ALLOC_PAGE_SIZE,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE
    );

    if (!uptr)
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
    void *uptr;

    uptr = _aligned_malloc(size, align);

    if (!uptr)
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
    HeapFree(__gauss_pgheap, 0, uptr);
    return;
}

void gauss_os_pfree(void *uptr, size_t size)
{
    if (!uptr)
        return;

    GAUSS_UNUSED(size);
    VirtualFree(uptr, 0, MEM_RELEASE);
    return;
}

void gauss_os_free_aligned(void *uptr, size_t size, size_t align)
{
    if (!uptr)
        return;

    GAUSS_UNUSED(size);
    GAUSS_UNUSED(align);
    _aligned_free(uptr);
    return;
}

uint16_t gauss_os_thread_id(void)
{
    return GetCurrentThreadId();
}

size_t gauss_os_page_size(void)
{
    SYSTEM_INFO si;

    GetSystemInfo(&si);

    return si.dwPageSize;
}

size_t gauss_os_cache_line_size(void)
{
    BOOL ret;
    DWORD size, loff;
    size_t cline;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION *slpi, *lptr;

    cline = GAUSS_CACHE_LINE_SIZE;

    GetLogicalProcessorInformation(NULL, &size);

    if (!size)
        goto __fallback;

    ret = GetLogicalProcessorInformation(slpi, &size);

    if (!ret)
        goto __release_slpi;

    for (loff = 0, lptr = slpi; loff < size; loff += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION)) {
        if (lptr->Relationship == RelationCache && lptr->Cache.Level == 1) {
            cline = lptr->Cache.LineSize;
            goto __release_slpi;
        }

        lptr++;
    }

__release_slpi:
    gauss_os_free(slpi, size);

__fallback:
    return cline;
}

uint32_t gauss_os_cpu_count(void)
{
    SYSTEM_INFO si;

    GetSystemInfo(&si);

    return si.dwNumberOfProcessors;
}

void gauss_os_memory_barrier(void)
{
    MemoryBarrier();
    return;
}

void gauss_os_sleep(uint64_t msecs)
{
    Sleep(msecs);
    return;
}

void *gauss_os_zero_buf(void *ubuf, size_t size)
{
    return SecureZeroMemory(ubuf, size);
}

#endif
