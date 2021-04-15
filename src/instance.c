#include "local.h"

static void * _default_malloc(void * ud, size_t size, size_t alignment) {
  UNUSED(ud);
  UNUSED(alignment);
  return malloc(size);
}

static void * _default_realloc(void * ud, void * ptr, size_t old_size, size_t new_size, size_t alignment) {
  UNUSED(ud);
  UNUSED(old_size);
  UNUSED(alignment);
  return realloc(ptr, new_size);
}

static void _default_free(void * ud, void * ptr, size_t size, size_t alignment) {
  UNUSED(ud);
  UNUSED(size);
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

  return_ERROR_INVALID_ARGUMENT_if(memory_callbacks->malloc == NULL);
  return_ERROR_INVALID_ARGUMENT_if(memory_callbacks->realloc == NULL);
  return_ERROR_INVALID_ARGUMENT_if(memory_callbacks->free == NULL);
  return_ERROR_INVALID_ARGUMENT_if(instance_ptr == NULL);

  aeInstance instance;
  instance = memory_callbacks->malloc(memory_callbacks->user_data, sizeof(*instance), alignof(*instance));
  if(instance == NULL) {
    return aeERROR_OUT_OF_MEMORY;
  }

  memset(instance, 0, sizeof(*instance));
  instance->mem = *memory_callbacks;

  *instance_ptr = instance;

  return aeSUCCESS;
}

void aeDestroyInstance(aeInstance instance) {
  void * user_data = instance->mem.user_data;
  void (*free)(void *, void *, size_t, size_t) = instance->mem.free;

  if(instance->layer.capacity > 0) {
    Vector_free(instance, &instance->layer.free_indexes);

    alias_ecs_free(instance, instance->layer.generation, sizeof(*instance->layer.generation) * instance->layer.capacity, alignof(*instance->layer.generation));

    for(uint32_t i = 0; i < instance->layer.length; i++) {
      Vector_free(instance, &instance->layer.data[i].entities);
    }
    alias_ecs_free(instance, instance->layer.data, sizeof(*instance->layer.data) * instance->layer.capacity, alignof(*instance->layer.data));
  }

  for(uint32_t i = 0; i < instance->component.length; i++) {
    FREE(instance, instance->component.data[i].num_required_components, instance->component.data[i].required_components);
  }
  Vector_free(instance, &instance->component);

  FREE(instance, instance->entity.capacity, instance->entity.generation);
  FREE(instance, instance->entity.capacity, instance->entity.layer_index);
  FREE(instance, instance->entity.capacity, instance->entity.archetype_index);
  FREE(instance, instance->entity.capacity, instance->entity.archetype_code);

  for(uint32_t i = 0; i < instance->archetype.length; i++) {
    struct aeArchetypeData * archetype = &instance->archetype.data[i];
    aeComponentSet_free(instance, &archetype->components);
    FREE(instance, archetype->components.count, archetype->offset_size);
    Vector_free(instance, &archetype->free_indexes);
    for(uint32_t j = 0; j < archetype->blocks.length; j++) {
      FREE(instance, 1, archetype->blocks.data[j]);
    }
    Vector_free(instance, &archetype->blocks);
  }
  FREE(instance, instance->archetype.capacity, instance->archetype.components_index);
  FREE(instance, instance->archetype.capacity, instance->archetype.data);
  
  free(user_data, instance, sizeof(*instance), alignof(*instance));
}

