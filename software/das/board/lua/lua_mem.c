
// change by jay 2016/12/09
// form rt-thread mem.c
// must less than

#include <rthw.h>
#include <rtthread.h>

#define RT_USING_LUA_RTU_MEM

#ifdef RT_USING_LUA_RTU_MEM

#define RT_LUA_RTU_MEM_DEBUG    0
#define RT_LUA_RTU_MEM_STATS
#define RT_LUA_RTU_ALIGN_SIZE   4

#define HEAP_MAGIC 0xde
struct heap_mem
{
    /* magic and used flag */
    rt_uint32_t magic   :8;
    rt_uint32_t next    :24;
    rt_uint32_t used    :8;
    rt_uint32_t prev    :24;
};

/** pointer to the heap: for alignment, heap_ptr is now a pointer instead of an array */
static rt_uint8_t *heap_ptr;

/** the last entry, always unused! */
static struct heap_mem *heap_end;

#define MIN_SIZE 8
#define MIN_SIZE_ALIGNED     RT_ALIGN(MIN_SIZE, RT_LUA_RTU_ALIGN_SIZE)
#define SIZEOF_STRUCT_MEM    RT_ALIGN(sizeof(struct heap_mem), RT_LUA_RTU_ALIGN_SIZE)

static struct heap_mem *lfree;   /* pointer to the lowest free block */

static struct rt_semaphore heap_sem;
static rt_size_t mem_size_aligned;

#ifdef RT_LUA_RTU_MEM_DEBUG
static rt_size_t used_mem, max_mem;
#endif

void *lua_malloc(rt_size_t nbytes);
void lua_free(void *ptr);
void *lua_realloc(void *ptr, rt_size_t nbytes);
void *lua_calloc(rt_size_t count, rt_size_t size);

static void plug_holes(struct heap_mem *mem)
{
    struct heap_mem *nmem;
    struct heap_mem *pmem;

    RT_ASSERT((rt_uint8_t *)mem >= heap_ptr);
    RT_ASSERT((rt_uint8_t *)mem < (rt_uint8_t *)heap_end);
    RT_ASSERT(mem->used == 0);

    nmem = (struct heap_mem *)&heap_ptr[mem->next];
    if (mem != nmem &&
        nmem->used == 0 &&
        (rt_uint8_t *)nmem != (rt_uint8_t *)heap_end)
    {
        if (lfree == nmem)
        {
            lfree = mem;
        }
        mem->next = nmem->next;
        ((struct heap_mem *)&heap_ptr[nmem->next])->prev = (rt_uint8_t *)mem - heap_ptr;
    }
    pmem = (struct heap_mem *)&heap_ptr[mem->prev];
    if (pmem != mem && pmem->used == 0)
    {
        if (lfree == mem)
        {
            lfree = pmem;
        }
        pmem->next = mem->next;
        ((struct heap_mem *)&heap_ptr[mem->next])->prev = (rt_uint8_t *)pmem - heap_ptr;
    }
}

void lua_system_heap_init(void *begin_addr, void *end_addr)
{
    struct heap_mem *mem;
    rt_uint32_t begin_align = RT_ALIGN((rt_uint32_t)begin_addr, RT_LUA_RTU_ALIGN_SIZE);
    rt_uint32_t end_align = RT_ALIGN_DOWN((rt_uint32_t)end_addr, RT_LUA_RTU_ALIGN_SIZE);

    RT_DEBUG_NOT_IN_INTERRUPT;

    if ((end_align > (2 * SIZEOF_STRUCT_MEM)) &&
        ((end_align - 2 * SIZEOF_STRUCT_MEM) >= begin_align))
    {
        /* calculate the aligned memory size */
        mem_size_aligned = end_align - begin_align - 2 * SIZEOF_STRUCT_MEM;
    }
    else
    {
        rt_kprintf("lua mem init, error begin address 0x%x, and end address 0x%x\n",
                   (rt_uint32_t)begin_addr, (rt_uint32_t)end_addr);

        return;
    }

    heap_ptr = (rt_uint8_t *)begin_align;

    RT_DEBUG_LOG(RT_LUA_RTU_MEM_DEBUG, ("lua mem init, heap begin address 0x%x, size %d\n",
                                (rt_uint32_t)heap_ptr, mem_size_aligned));

    mem        = (struct heap_mem *)heap_ptr;
    mem->magic = HEAP_MAGIC;
    mem->next  = mem_size_aligned + SIZEOF_STRUCT_MEM;
    mem->prev  = 0;
    mem->used  = 0;

    heap_end        = (struct heap_mem *)&heap_ptr[mem->next];
    heap_end->magic = HEAP_MAGIC;
    heap_end->used  = 1;
    heap_end->next  = mem_size_aligned + SIZEOF_STRUCT_MEM;
    heap_end->prev  = mem_size_aligned + SIZEOF_STRUCT_MEM;

    rt_sem_init(&heap_sem, "lrtuheap", 1, RT_IPC_FLAG_FIFO);

    lfree = (struct heap_mem *)heap_ptr;
}
RTM_EXPORT(lua_system_heap_init);

void *lua_malloc(rt_size_t size)
{
    rt_size_t ptr, ptr2;
    struct heap_mem *mem, *mem2;

    RT_DEBUG_NOT_IN_INTERRUPT;

    if (size == 0)
        return RT_NULL;

    if (size != RT_ALIGN(size, RT_LUA_RTU_ALIGN_SIZE))
        RT_DEBUG_LOG(RT_LUA_RTU_MEM_DEBUG, ("lua malloc size %d, but align to %d\n",
                                    size, RT_ALIGN(size, RT_LUA_RTU_ALIGN_SIZE)));
    else
        RT_DEBUG_LOG(RT_LUA_RTU_MEM_DEBUG, ("lua malloc size %d\n", size));

    /* alignment size */
    size = RT_ALIGN(size, RT_LUA_RTU_ALIGN_SIZE);

    if (size > mem_size_aligned)
    {
        RT_DEBUG_LOG(RT_LUA_RTU_MEM_DEBUG, ("lua no memory\n"));

        return RT_NULL;
    }

    if (size < MIN_SIZE_ALIGNED)
        size = MIN_SIZE_ALIGNED;

    rt_sem_take(&heap_sem, RT_WAITING_FOREVER);

    for (ptr = (rt_uint8_t *)lfree - heap_ptr;
         ptr < mem_size_aligned - size;
         ptr = ((struct heap_mem *)&heap_ptr[ptr])->next)
    {
        mem = (struct heap_mem *)&heap_ptr[ptr];

        if ((!mem->used) && (mem->next - (ptr + SIZEOF_STRUCT_MEM)) >= size)
        {
            if (mem->next - (ptr + SIZEOF_STRUCT_MEM) >=
                (size + SIZEOF_STRUCT_MEM + MIN_SIZE_ALIGNED))
            {
                ptr2 = ptr + SIZEOF_STRUCT_MEM + size;

                /* create mem2 struct */
                mem2       = (struct heap_mem *)&heap_ptr[ptr2];
                mem2->used = 0;
                mem2->next = mem->next;
                mem2->prev = ptr;

                /* and insert it between mem and mem->next */
                mem->next = ptr2;
                mem->used = 1;

                if (mem2->next != mem_size_aligned + SIZEOF_STRUCT_MEM)
                {
                    ((struct heap_mem *)&heap_ptr[mem2->next])->prev = ptr2;
                }
#ifdef RT_LUA_RTU_MEM_DEBUG
                used_mem += (size + SIZEOF_STRUCT_MEM);
                if (max_mem < used_mem)
                    max_mem = used_mem;
#endif
            }
            else
            {
                mem->used = 1;
#ifdef RT_LUA_RTU_MEM_DEBUG
                used_mem += mem->next - ((rt_uint8_t*)mem - heap_ptr);
                if (max_mem < used_mem)
                    max_mem = used_mem;
#endif
            }
            /* set memory block magic */
            mem->magic = HEAP_MAGIC;

            if (mem == lfree)
            {
                /* Find next free block after mem and update lowest free pointer */
                while (lfree->used && lfree != heap_end)
                    lfree = (struct heap_mem *)&heap_ptr[lfree->next];

                RT_ASSERT(((lfree == heap_end) || (!lfree->used)));
            }

            rt_sem_release(&heap_sem);
            RT_ASSERT((rt_uint32_t)mem + SIZEOF_STRUCT_MEM + size <= (rt_uint32_t)heap_end);
            RT_ASSERT((rt_uint32_t)((rt_uint8_t *)mem + SIZEOF_STRUCT_MEM) % RT_LUA_RTU_ALIGN_SIZE == 0);
            RT_ASSERT((((rt_uint32_t)mem) & (RT_LUA_RTU_ALIGN_SIZE-1)) == 0);

            RT_DEBUG_LOG(RT_LUA_RTU_MEM_DEBUG,
                         ("lua [%s] allocate memory at 0x%x, size: %d\n",
                          rt_thread_self()->name,
                          (rt_uint32_t)((rt_uint8_t *)mem + SIZEOF_STRUCT_MEM),
                          (rt_uint32_t)(mem->next - ((rt_uint8_t *)mem - heap_ptr))));

            /* return the memory data except mem struct */
            return (rt_uint8_t *)mem + SIZEOF_STRUCT_MEM;
        }
    }

    rt_sem_release(&heap_sem);

    return RT_NULL;
}
RTM_EXPORT(lua_malloc);

void *lua_realloc(void *rmem, rt_size_t newsize)
{
    rt_size_t size;
    rt_size_t ptr, ptr2;
    struct heap_mem *mem, *mem2;
    void *nmem;

    RT_DEBUG_NOT_IN_INTERRUPT;

    /* alignment size */
    newsize = RT_ALIGN(newsize, RT_LUA_RTU_ALIGN_SIZE);
    if (newsize > mem_size_aligned)
    {
        RT_DEBUG_LOG(RT_LUA_RTU_MEM_DEBUG, ("lua realloc: out of memory\n"));

        return RT_NULL;
    }

    /* allocate a new memory block */
    if (rmem == RT_NULL)
        return lua_malloc(newsize);

    rt_sem_take(&heap_sem, RT_WAITING_FOREVER);

    if ((rt_uint8_t *)rmem < (rt_uint8_t *)heap_ptr ||
        (rt_uint8_t *)rmem >= (rt_uint8_t *)heap_end)
    {
        /* illegal memory */
        rt_sem_release(&heap_sem);

        return rmem;
    }

    mem = (struct heap_mem *)((rt_uint8_t *)rmem - SIZEOF_STRUCT_MEM);

    ptr = (rt_uint8_t *)mem - heap_ptr;
    size = mem->next - ptr - SIZEOF_STRUCT_MEM;
    if (size == newsize)
    {
        /* the size is the same as */
        rt_sem_release(&heap_sem);

        return rmem;
    }

    if (newsize + SIZEOF_STRUCT_MEM + MIN_SIZE < size)
    {
        /* split memory block */
#ifdef RT_LUA_RTU_MEM_DEBUG
        used_mem -= (size - newsize);
#endif

        ptr2 = ptr + SIZEOF_STRUCT_MEM + newsize;
        mem2 = (struct heap_mem *)&heap_ptr[ptr2];
        mem2->magic= HEAP_MAGIC;
        mem2->used = 0;
        mem2->next = mem->next;
        mem2->prev = ptr;
        mem->next = ptr2;
        if (mem2->next != mem_size_aligned + SIZEOF_STRUCT_MEM)
        {
            ((struct heap_mem *)&heap_ptr[mem2->next])->prev = ptr2;
        }

        plug_holes(mem2);

        rt_sem_release(&heap_sem);

        return rmem;
    }
    rt_sem_release(&heap_sem);

    /* expand memory */
    nmem = lua_malloc(newsize);
    if (nmem != RT_NULL) /* check memory */
    {
        rt_memcpy(nmem, rmem, size < newsize ? size : newsize);
        lua_free(rmem);
    }

    return nmem;
}
RTM_EXPORT(lua_realloc);

void *lua_calloc(rt_size_t count, rt_size_t size)
{
    void *p;

    RT_DEBUG_NOT_IN_INTERRUPT;

    /* allocate 'count' objects of size 'size' */
    p = lua_malloc(count * size);

    /* zero the memory */
    if (p)
        rt_memset(p, 0, count * size);

    return p;
}
RTM_EXPORT(lua_calloc);

void lua_free(void *rmem)
{
    struct heap_mem *mem;

    RT_DEBUG_NOT_IN_INTERRUPT;
    
    if (rmem == RT_NULL)
        return;
    RT_ASSERT((((rt_uint32_t)rmem) & (RT_LUA_RTU_ALIGN_SIZE-1)) == 0);
    RT_ASSERT((rt_uint8_t *)rmem >= (rt_uint8_t *)heap_ptr &&
              (rt_uint8_t *)rmem < (rt_uint8_t *)heap_end);

    if ((rt_uint8_t *)rmem < (rt_uint8_t *)heap_ptr ||
        (rt_uint8_t *)rmem >= (rt_uint8_t *)heap_end)
    {
        RT_DEBUG_LOG(RT_LUA_RTU_MEM_DEBUG, ("lua illegal memory\n"));

        return;
    }

    /* Get the corresponding struct heap_mem ... */
    mem = (struct heap_mem *)((rt_uint8_t *)rmem - SIZEOF_STRUCT_MEM);

    RT_DEBUG_LOG(RT_LUA_RTU_MEM_DEBUG,
                 ("lua release memory 0x%x, size: %d\n",
                  (rt_uint32_t)rmem,
                  (rt_uint32_t)(mem->next - ((rt_uint8_t *)mem - heap_ptr))));


    /* protect the heap from concurrent access */
    rt_sem_take(&heap_sem, RT_WAITING_FOREVER);

    /* ... which has to be in a used state ... */
    RT_ASSERT(mem->used);
    RT_ASSERT(mem->magic == HEAP_MAGIC);
    /* ... and is now unused. */
    mem->used  = 0;
    mem->magic = 0;

    if (mem < lfree)
    {
        /* the newly freed struct is now the lowest */
        lfree = mem;
    }

#ifdef RT_LUA_RTU_MEM_DEBUG
    used_mem -= (mem->next - ((rt_uint8_t*)mem - heap_ptr));
#endif

    /* finally, see if prev or next are free also */
    plug_holes(mem);
    rt_sem_release(&heap_sem);
}
RTM_EXPORT(lua_free);

#ifdef RT_LUA_RTU_MEM_DEBUG
void lua_memory_info(rt_uint32_t *total,
                    rt_uint32_t *used,
                    rt_uint32_t *max_used)
{
    if (total != RT_NULL)
        *total = mem_size_aligned;
    if (used  != RT_NULL)
        *used = used_mem;
    if (max_used != RT_NULL)
        *max_used = max_mem;
}

#ifdef RT_USING_FINSH
#include <finsh.h>

void list_lua_mem(void)
{
    rt_kprintf("lua total memory: %d\n", mem_size_aligned);
    rt_kprintf("lua used memory : %d\n", used_mem);
    rt_kprintf("lua free memory : %d\n", mem_size_aligned-used_mem);
    rt_kprintf("lua maximum allocated memory: %d\n", max_mem);
}
FINSH_FUNCTION_EXPORT(list_lua_mem, list lua memory usage information)
#endif
#endif

/*@}*/

#endif /* end of RT_USING_LUA_RTU_MEM */

