#ifndef _ALIAS_ECS_LOCAL_H_
#define _ALIAS_ECS_LOCAL_H_

#include <alias/ecs.h>

#include <stdlib.h>
#include <stdalign.h>
#include <string.h>

#define UNUSED(X) (void)X
#define sizeof_alignof(X) sizeof(X), alignof(X)

typedef uint32_t aeArchetype;

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

struct aeComponentSet {
  uint32_t count;
  uint32_t * index;
};

#define TOTAL_BLOCK_SIZE (1 << 16)
#define BLOCK_DATA_SIZE (TOTAL_BLOCK_SIZE /* - (sizeof(uint16_t) * 2) */)

struct aeDataBlock {
  //uint16_t live_count;
  //uint16_t fill_count;
  uint8_t data[BLOCK_DATA_SIZE];
};

struct aeArchetypeData {
  struct aeComponentSet components;

  uint32_t * offset_size;

  uint32_t entity_size;
  uint32_t entities_per_block;

  uint32_t next_index;
  struct Vector(uint32_t) free_indexes;

  struct Vector(struct aeDataBlock *) blocks;
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

  // archetypes are 'sorted' by component sets
  // created as needed
  struct {
    uint32_t capacity;
    uint32_t length;
    uint32_t * components_index;
    struct aeArchetypeData * data;
  } archetype;

  struct Vector(struct aeComponentData) component;
};

// ============================================================================
#define ENTITY_GENERATION(I, E)             (I)->entity.generation[E]
#define ENTITy_LAYER_INDEX(I, E)            (I)->entity.layer_index[E]
#define ENTITY_ARCHETYPE_INDEX(I, E)        (I)->entity.archetype_index[E]
#define ENTITY_ARCHETYPE_CODE(I, E)         (I)->entity.archetype_code[E]
#define ENTITY_ARCHETYPE_BLOCK_INDEX(I, E)  (ENTITY_ARCHETYPE_CODE(I, E) >> 16)
#define ENTITY_ARCHETYPE_BLOCK_OFFSET(I, E) (ENTITY_ARCHETYPE_CODE(I, E) & 0xFFFF)
#define ENTITY_ARCHETYPE_DATA(I, E)         (&(I)->archetype.data[ENTITY_ARCHETYPE_INDEX(I, E)])
#define ENTITY_DATA_BLOCK(I, E)             ENTITY_ARCHETYPE_DATA(I, E)->blocks.data[ENTITY_ARCHETYPE_BLOCK_INDEX(I, E)]
#define ENTITY_DATA_BLOCK_DATA(I, E)        (ENTITY_DATA_BLOCK(I, E)->data)
#define ENTITY_DATA_ENTITY_INDEX(I, E)      ((uint32_t *)ENTITY_DATA_BLOCK_DATA(I, E))[ENTITY_ARCHETYPE_BLOCK_OFFSET(I, E)]

static inline void * alias_ecs_raw_access(aeInstance instance, uint32_t archetype_index, uint32_t component_index, uint32_t block_index, uint32_t block_offset) {
  struct aeArchetypeData * archetype = &instance->archetype.data[archetype_index];
  struct aeDataBlock * block = archetype->blocks.data[block_index];
  uint32_t offset_size = archetype->offset_size[component_index];
  uint32_t offset = offset_size >> 16;
  uint32_t size = offset_size & 0xFFFF;
  return (void *)(block->data + (uintptr_t)offset + ((uintptr_t)block_offset * size));
}

// ============================================================================
#define return_if_ERROR(C) do { aeResult __r = C; if(__r < aeSUCCESS) return __r; } while(0)
#define return_ERROR_INVALID_ARGUMENT_if(X) do { if(X) { return aeERROR_INVALID_ARGUMENT; } } while(0)

// ============================================================================
// memory.c
aeResult alias_ecs_malloc(aeInstance instance, size_t size, size_t alignment, void ** out_ptr);
aeResult alias_ecs_realloc(aeInstance instance, void * ptr, size_t old_size, size_t new_size, size_t alignment, void ** out_ptr);
void alias_ecs_free(aeInstance instance, void * ptr, size_t size, size_t alignment);

void alias_ecs_quicksort(void * base, size_t num, size_t size, int (*compar)(void *, const void *, const void *), void * ud);
void * alias_ecs_bsearch(const void * key, const void * base, size_t num, size_t size, int (*compar)(void *, const void *, const void *), void * ud);

#define ALLOC(I, C, P)          return_if_ERROR(alias_ecs_malloc(I, (C) * sizeof_alignof(*P), (void **)&P))
#define RELOC(I, oldC, newC, P) return_if_ERROR(alias_ecs_realloc(I, P, (oldC) * sizeof(*P), (newC) * sizeof_alignof(*P), (void **)&P))
#define FREE(I, C, P)           alias_ecs_free(I, (void *)P, (C) * sizeof_alignof(*P))

// ============================================================================
// component.c
aeResult aeComponentSet_init(aeInstance instance, struct aeComponentSet * set, uint32_t count, const aeComponent * components);
aeResult aeComponentSet_add(aeInstance instance, struct aeComponentSet * dst, const struct aeComponentSet * src, aeComponent component);
aeResult aeComponentset_remove(aeInstance instance, struct aeComponentSet * dst, const struct aeComponentSet * src, aeComponent component);
uint32_t aeComponentSet_order_of(const struct aeComponentSet * set, aeComponent component);
int aeComponentSet_contains(const struct aeComponentSet * set, aeComponent component);
int aeComponentSet_intersects(const struct aeComponentSet * a, const struct aeComponentSet * b);
void aeComponentSet_free(aeInstance instance, struct aeComponentSet * set);

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

// ============================================================================
// archetype.c
aeResult alias_ecs_resolve_archetype(aeInstance instance, struct aeComponentSet components, aeArchetype * out_ptr);
aeResult alias_ecs_unset_archetype(aeInstance instance, uint32_t entity_index);
aeResult alias_ecs_set_archetype(aeInstance instance, uint32_t entity_index, uint32_t archetype_index);

#endif

