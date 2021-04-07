#include "local.h"

aeResult alias_ecs_entity_validate(const aeInstance instance, aeEntity entity, uint32_t * index_ptr) {
  uint32_t index = (uint32_t)(entity & 0xFFFFFFFF);
  uint32_t generation = (uint32_t)(entity >> 31);
  if(index >= instance->entity.length) {
    return aeERROR_INVALID_ENTITY;
  }
  if(generation != instance->entity.generation[*index_ptr]) {
    return aeERROR_INVALID_ENTITY;
  }
  return aeSUCCESS;
}

aeResult alias_ecs_entity_create(aeInstance instance, aeEntity * entity_ptr) {
  uint32_t index;

  if(instance->entity.free_indexes.length > 0) {
    index = *Vector_pop(&instance->entity.free_indexes);
  } else {
    index = instance->entity.length++;
  }

  if(instance->entity.length > instance->entity.capacity) {
    size_t old_capacity = instance->entity.capacity;
    size_t new_capacity = instance->entity.length + 1;
    new_capacity += new_capacity >> 1;
    // TODO better cleanup
    #define REALLOC(P) return_if_ERROR(alias_ecs_realloc(instance, instance->entity.generation, sizeof(*P) * old_capacity, sizeof(*P) * new_capacity, alignof(*P), (void **)&P))
    REALLOC(instance->entity.generation);
    REALLOC(instance->entity.layer_index);
    REALLOC(instance->entity.archetype_index);
    REALLOC(instance->entity.archetype_code);
    #undef REALLOC
    instance->entity.capacity = new_capacity;
  }

  uint32_t generation = instance->entity.generation[index];

  *entity_ptr = ((uint64_t)generation << 32) | (uint64_t)index;

  return aeSUCCESS;
}

aeResult alias_ecs_entity_free(aeInstance instance, uint32_t entity_id) {
  ++instance->entity.generation[entity_id];
  return_if_ERROR(Vector_space_for(instance, &instance->entity.free_indexes, 1));
  *Vector_push(&instance->entity.free_indexes) = entity_id;
  return aeSUCCESS;
}
