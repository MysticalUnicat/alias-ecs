#include "local.h"

#include <string.h>

aeResult alias_ecs_malloc(aeInstance instance, size_t size, size_t alignment, void ** out_ptr) {
  *out_ptr =instance->mem.malloc(instance->mem.user_data, size, alignment);
  if(*out_ptr == NULL) {
    return aeERROR_OUT_OF_MEMORY;
  }
  memset(*out_ptr, 0, size);
  return aeSUCCESS;
}

aeResult alias_ecs_realloc(aeInstance instance, void * ptr, size_t old_size, size_t new_size, size_t alignment, void ** out_ptr) {
  *out_ptr = instance->mem.realloc(instance->mem.user_data, ptr, old_size, new_size, alignment);
  if(*out_ptr == NULL) {
    return aeERROR_OUT_OF_MEMORY;
  }
  if(old_size < new_size) {
    memset(((unsigned char *)*out_ptr) + old_size, 0, new_size - old_size);
  }
  return aeSUCCESS;
}

void alias_ecs_free(aeInstance instance, void * ptr, size_t size, size_t alignment) {
  instance->mem.free(instance->mem.user_data, ptr, size, alignment);
}

// naive qsort
// https://en.wikipedia.org/wiki/Quicksort

#define OPTIMIZED
#define QSORT_LINEAR_SORT_LIMIT 16

static inline void _swap(uint8_t * base, size_t size, size_t i, size_t j) {
  if(i == j) {
    return;
  }

  uint8_t * a = base + size * i;
  uint8_t * b = base + size * j;

  for(size_t i = 0; i < size; i++, a++, b++) {
    uint8_t temp = *a;
    *a = *b;
    *b = temp;
  } while(size-- > 0);
}
#define SWP(I, J) _swap(base, size, I, J)

enum PivotScheme {
  PivotLo,
  PivotHi,
  PivotMid,
  MedianOfThree
};

enum PartitionScheme {
  Lumuto,
  Hoare
};

static int _cmp(const uint8_t * base, size_t size, size_t i, size_t j, int (*compar)(void *, const void *, const void *), void * ud) {
  if(i == j) {
    return 0;
  }
  
  return compar(ud, base + (size * i), base + (size * j));
}
#define CMP(I, OP, J) (_cmp(base, size, I, J, compar, ud) OP 0)

static inline size_t size_t_min(size_t a, size_t b) {
  return a < b ? a : b;
}

static size_t _partition(enum PivotScheme pivot_scheme, enum PartitionScheme partition_scheme, uint8_t * base, size_t size, size_t lo, size_t hi
                        , int (*compar)(void *, const void *, const void *), void * ud) {
  size_t pivot, mid = (hi >> 1) + (lo >> 1);

  switch(pivot_scheme) {
  case PivotLo:
    pivot = lo;
    break;
  case PivotHi:
    pivot = hi;
    break;
  case PivotMid:
    pivot = mid;
    break;
  case MedianOfThree:
    if(CMP(mid, <, lo)) {
      SWP(lo, mid);
    }
    if(CMP(hi, <, lo)) {
      SWP(lo, hi);
    }
    if(CMP(mid, <, hi)) {
      SWP(mid, hi);
    }
    pivot = hi;
    break;
  }

  size_t i, j;

  switch(partition_scheme) {
  case Lumuto:
    if(pivot != hi) {
      SWP(pivot, hi);
      pivot = hi;
    }
    i = lo;
    for(size_t j = lo; j < hi; j++) {
      if(CMP(j, <, pivot)) {
        SWP(i, j);
        i++;
      }
    }
    SWP(i, pivot);
    return i;
  case Hoare:
    i = lo;
    j = hi;
    for(;;) {
      while(CMP(i, <, pivot)) {
        i++;
      }
      while(CMP(j, >, pivot)) {
        j--;
      }
      if(i >= j) {
        return j;
      }
      SWP(i, j);
      pivot = i == pivot ? j :
              j == pivot ? i : pivot;
    }
  }

  // does not reach here
  return 0;
}

typedef void (*SortFn)(uint8_t * base, size_t size, size_t lo, size_t hi, int (*compar)(void *, const void *, const void *), void *);

#ifndef OPTIMIZED
static void _quicksort(uint8_t * base, size_t size, size_t lo, size_t hi, int (*compar)(void *, const void *, const void *), void * ud) {
  if(lo >= hi) {
    return;
  }
  size_t pivot = _partition(PivotMid, Hoare, base, size, lo, hi, compar, ud);
  _quicksort(base, size, lo, pivot ? pivot - 1 : pivot, compar, ud);
  _quicksort(base, size, pivot + 1, hi, compar, ud);
}
#else
static void _insertion_sort(uint8_t * base, size_t size, size_t lo, size_t hi, int (*compar)(void *, const void *, const void *), void * ud) {
  size_t i = lo + 1;
  while(i <= hi) {
    size_t j = i;
    while(j > 0 && CMP(j - 1, >, j)) {
      SWP(j, j - 1);
      j--;
    }
    i++;
  }
}

static void _quicksort(uint8_t * base, size_t size, size_t lo, size_t hi, int (*compar)(void *, const void *, const void *), void * ud) {
again:
  if(lo >= hi) {
    return;
  }

  // optimization: perform linear insertion sort on small sub arrays
  if(hi - lo < QSORT_LINEAR_SORT_LIMIT) {
    _insertion_sort(base, size, lo, hi, compar, ud);
    return;
  }

  size_t pivot = _partition(PivotHi, Lumuto, base, size, lo, hi, compar, ud);
  
  // optimization: sort the smaller subarray first (in the call stack) then go to the top with the other side
  size_t sub[2][2] = { {    lo, pivot ? pivot - 1 : lo }
                     , { pivot + 1,    hi }
                     };
  size_t shortest = (sub[0][1] - sub[0][0] > sub[1][1] - sub[1][0]);
  _quicksort(base, size, sub[shortest][0], sub[shortest][1], compar, ud);
  lo = sub[!shortest][0];
  hi = sub[!shortest][1];
  goto again;
}
#endif

void alias_ecs_quicksort(void * base, size_t num, size_t size, int (*compar)(void *, const void *, const void *), void * ud) {
  if(num == 0 || size == 0 || base == NULL || compar == NULL) {
    return;
  }
  _quicksort((uint8_t *)base, size, 0, num - 1, compar, ud);
}

// naive bsearch
void * alias_ecs_bsearch(const void * key, const void * base, size_t num, size_t size, int (*compar)(void *, const void *, const void *), void * ud) {
  const uint8_t * b = (const uint8_t *)base;
  size_t lo = 0;
  size_t hi = num - 1;
  while(lo <= hi) {
    size_t mid = (lo >> 1) + (hi >> 1);
    const void * item = b + mid * size;
    int cmp = compar(ud, key, item);
    if(cmp < 0) {
      lo = mid + 1;
    } else if(cmp > 0) {
      hi = mid - 1;
    } else {
      return (void *)item;
    }
  }
  return NULL;
}

