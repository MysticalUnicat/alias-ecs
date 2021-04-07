#ifndef _ALIAS_ECS_LOCAL_H_
#define _ALIAS_ECS_LOCAL_H_

#include <alias/ecs.h>

#include <stdlib.h>
#include <stdalign.h>
#include <string.h>

#define UNUSED(X) (void)X

#define Vector(T) { uint32_t capacity; uint32_t length; T * data; }

struct aeLayerData {
  uint32_t dirty : 1;
  uint32_t at_max : 1;
  uint32_t _reserved : 30;
  struct Vector(uint32_t) entities;
};

struct aeComponentData {
  uint32_t size;
  uint32_t num_required_components;
  const aeComponent * required_components;
};

struct aeInstance {
  aliasApplicationMemoryCallbacks mem;

  // generational layers
  struct {
    struct Vector(uint32_t) free_indexes;
    uint32_t capacity;
    uint32_t length;
    uint32_t * generation;
    struct aeLayerData * data;
  } layer;

  // generational entities
  struct {
    struct Vector(uint32_t) free_indexes;
    uint32_t capacity;
    uint32_t length;
    uint32_t * generation;
    uint32_t * layer_index;
    uint32_t * archetype_index;
    uint32_t * archetype_code;
  } entity;

  struct Vector(struct aeComponentData) component;
};

// ============================================================================
#define return_if_ERROR(C) do { aeResult __r = C; if(__r < aeSUCCESS) return __r; } while(0)
#define return_ERROR_INVALID_ARGUMENT_if(X) do { if(X) { return aeERROR_INVALID_ARGUMENT; } } while(0)

// ============================================================================
// memory.c
aeResult alias_ecs_malloc(aeInstance instance, size_t size, size_t alignment, void ** out_ptr);
aeResult alias_ecs_realloc(aeInstance instance, void * ptr, size_t old_size, size_t new_size, size_t alignment, void ** out_ptr);
void alias_ecs_free(aeInstance instance, void * ptr, size_t size, size_t alignment);

// ============================================================================
// Vector utility functions and macros

// VoidVector makes some code easier
typedef struct Vector(void) VoidVector;
static inline aeResult _VoidVector_set_capacity(aeInstance instance, VoidVector * vv, size_t s, size_t a, size_t new_capacity) {
  size_t old_capacity = vv->capacity;
  if(old_capacity == new_capacity) {
    return aeSUCCESS;
  }
  aeResult result = alias_ecs_realloc(instance, vv->data, old_capacity * s, new_capacity * s, a, &vv->data);
  if(result >= aeSUCCESS) {
    vv->capacity = new_capacity;
  }
  return result;
}
static inline aeResult _VoidVector_space_for(aeInstance instance, VoidVector * vv, size_t s, size_t a, size_t c) {
  size_t new_capacity = vv->length + c;
  if(new_capacity > vv->capacity) {
    new_capacity += new_capacity >> 1;
    return  _VoidVector_set_capacity(instance, vv, s, a, new_capacity);
  }
  return aeSUCCESS;
}
#define _Vector_VoidVector_args(V) (VoidVector *)(V), sizeof(*(V)->data), alignof(*(V)->data)

#define Vector_free(I, V)                                                                    \
  do {                                                                                       \
    if((V)->data != NULL) {                                                                  \
      alias_ecs_free(I, (V)->data, (V)->capacity * sizeof(*(V)->data), alignof(*(V)->data)); \
    }                                                                                        \
    (V)->length = 0;                                                                         \
    (V)->capacity = 0;                                                                       \
    (V)->data = NULL;                                                                        \
  } while(0)
#define Vector_pop(V) ((V)->data + (--(V)->length))
#define Vector_push(V) ((V)->data + ((V)->length++))
#define Vector_set_capacity(I, V, C) _VoidVector_set_capacity(I, _Vector_VoidVector_args(V), C)
#define Vector_space_for(I, V, C) _VoidVector_space_for(I, _Vector_VoidVector_args(V), C)
#define Vector_qsort(V, F) qsort((V)->data, (V)->length, sizeof(*(V)->data), F)
#define Vector_bsearch(V, F, K) bsearch(K, (V)->data, (V)->length, sizeof(*(V)->data), F)
#define Vector_remove_at(V, I)                                                                   \
  do {                                                                                           \
      uint32_t __i = (I);                                                                        \
      (V)->length--;                                                                             \
      if((V)->length > 0 && (V)->length != __i) {                                                \
        memmove((V)->data + __i, (V)->data + __i + 1, ((V)->length - __i) * sizeof(*(V)->data)); \
      }                                                                                          \
  } while(0)
#define Vector_swap_pop(V, I)                                               \
  do {                                                                      \
    uint32_t __i = (I);                                                     \
    (V)->length--;                                                          \
    if((V)->length != __i) {                                                \
      memcpy((V)->data + __i, (V)->data + (V)->length, sizeof(*(V)->data)); \
    }                                                                       \
  } while(0)

// ============================================================================
// layer.c
aeResult alias_ecs_layer_validate(const aeInstance instance, aeLayer layer, uint32_t * index_ptr);
void alias_ecs_unset_layer(aeInstance instance, uint32_t entity_index);
aeResult alias_ecs_set_layer(aeInstance instance, uint32_t entity_index, uint32_t layer_index);

// ============================================================================
// entity.c
aeResult alias_ecs_entity_validate(const aeInstance instance, aeEntity entity, uint32_t * index_ptr);
aeResult alias_ecs_entity_create(aeInstance instance, aeEntity * entity_ptr);
aeResult alias_ecs_entity_free(aeInstance instance, uint32_t entity_id);

#endif

