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

