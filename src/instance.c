#include "local.h"

#include <stdlib.h>
#include <stdalign.h>

static void * _default_malloc(void * ud, size_t size, size_t alignment) {
  UNUSED(ud);
  UNUSED(alignment);
  void * p = malloc(size);
  if(p == NULL) {
    return NULL;
  } else {
    return p;
  }
}

static void * _default_realloc(void * ud, void * ptr, size_t old_size, size_t new_size, size_t alignment) {
  UNUSED(ud);
  UNUSED(old_size);
  UNUSED(alignment);
  return realloc(ptr, new_size);
}

static void _default_free(void * ud, void * ptr, size_t alignment) {
  UNUSED(ud);
  UNUSED(alignment);
  free(ptr);
}

static aliasApplicationMemoryCallbacks _default_memory_callbacks =
  { .malloc = _default_malloc
  , .realloc = _default_realloc
  , .free = _default_free
  , .user_data = NULL
  };

aeResult aeCreateInstance(const aliasApplicationMemoryCallbacks * memory_callbacks, aeInstance * instance_ptr) {
  if(memory_callbacks == NULL) {
    memory_callbacks = &_default_memory_callbacks;
  }

  if(memory_callbacks->malloc == NULL || memory_callbacks->realloc == NULL || memory_callbacks->free == NULL) {
    return aeERROR_INVALID_ARGUMENT;
  }

  if(instance_ptr == NULL) {
    return aeERROR_INVALID_ARGUMENT;
  }

  aeInstance instance;
  instance = memory_callbacks->malloc(memory_callbacks->user_data, sizeof(*instance), alignof(*instance));
  if(instance == NULL) {
    return aeERROR_OUT_OF_MEMORY;
  }

  instance->mem = *memory_callbacks;

  *instance_ptr = instance;

  return aeSUCCESS;
}

void aeDestroyInstance(aeInstance instance) {
  void * user_data = instance->mem.user_data;
  void (*free)(void *, void *, size_t) = instance->mem.free;
  free(user_data, instance, alignof(*instance));
}

