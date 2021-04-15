#include "local.h"

aeResult alias_ecs_entity_validate(const aeInstance instance, aeEntity entity, uint32_t * index_ptr) {
  uint32_t index = (uint32_t)(entity & 0xFFFFFFFF);
  uint32_t generation = (uint32_t)(entity >> 31);
  if(index >= instance->entity.length) {
    return aeERROR_INVALID_ENTITY;
  }
  if(generation != instance->entity.generation[index]) {
    return aeERROR_INVALID_ENTITY;
  }
  *index_ptr = index;
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
    RELOC(instance, old_capacity, new_capacity, instance->entity.generation);
    RELOC(instance, old_capacity, new_capacity, instance->entity.layer_index);
    RELOC(instance, old_capacity, new_capacity, instance->entity.archetype_index);
    RELOC(instance, old_capacity, new_capacity, instance->entity.archetype_code);
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

// -------------------------------------------------------------------------------------------------------------------------------------------------------------

static int _compar_component_index(const void * ap, const void * bp) {
  uint32_t a = *(uint32_t *)ap;
  uint32_t b = *(uint32_t *)bp;
  return a - b;
}

aeResult aeSpawn(aeInstance instance, const aeEntitySpawnInfo * spawn_info, aeEntity * entities_ptr) {
  return_ERROR_INVALID_ARGUMENT_if(instance == NULL);
  return_ERROR_INVALID_ARGUMENT_if(spawn_info == NULL);

  return_ERROR_INVALID_ARGUMENT_if(spawn_info->count == 0);

  uint32_t layer_index = UINT32_MAX;
  if(spawn_info->layer != aeINVALID_LAYER) {
    return_if_ERROR(alias_ecs_layer_validate(instance, spawn_info->layer, &layer_index));
  }

  aeArchetype archetype_index;
  {
    struct aeComponentSet components;
    components.count = spawn_info->num_components;
    ALLOC(instance, components.count, components.index);
    for(uint32_t i = 0; i < components.count; ++i) {
      components.index[i] = spawn_info->components[i].component;
    }
    qsort(components.index, components.count, sizeof(*components.index), _compar_component_index);
    return_if_ERROR(alias_ecs_resolve_archetype(instance, components, &archetype_index));
  }
  struct aeArchetypeData * archetype = &instance->archetype.data[archetype_index];

  int free_out_entities = entities_ptr == NULL;
  if(free_out_entities) {
    ALLOC(instance, spawn_info->count, entities_ptr);
  }

  for(uint32_t i = 0; i < spawn_info->count; i++) {
    aeEntity entity;
    uint32_t entity_index;

    return_if_ERROR(alias_ecs_entity_create(instance, &entity));
    return_if_ERROR(alias_ecs_entity_validate(instance, entity, &entity_index));
    if(layer_index != UINT32_MAX) {
      return_if_ERROR(alias_ecs_set_layer(instance, layer_index, entity_index));
    }
    return_if_ERROR(alias_ecs_set_archetype(instance, archetype_index, entity_index));

    entities_ptr[i] = entity;
  }

  for(uint32_t i = 0; i < spawn_info->num_components; i++) {
    aeEntitySpawnComponent spawn_component = spawn_info->components[i];

    uint32_t component_index = aeComponentSet_order_of(&archetype->components, spawn_component.component);

    // ASSERT(component_index != UINT32_MAX);

    uint32_t offset_size = archetype->offset_size[component_index];
    uint32_t component_offset = offset_size >> 16;
    uint32_t component_size = offset_size & 0xFFFF;

    const uint8_t * read = spawn_component.data;
    uint32_t stride = spawn_component.stride ? spawn_component.stride : component_size;

    for(uint32_t j = 0; j < spawn_info->count; j++) {
      uint32_t entity_index = (uint32_t)(entities_ptr[j] & 0xFFFFFFFF);
      uint32_t code = ENTITY_ARCHETYPE_CODE(instance, entity_index);
      uint32_t block_index = code >> 16;
      uint32_t block_offset = code & 0xFFFF;
      void * write = (void *)(archetype->blocks.data[block_index]->data + component_offset + (component_size * block_offset));
      memcpy(write, read, component_size);
      read += stride;
    }
  }

  if(free_out_entities) {
    FREE(instance, spawn_info->count, entities_ptr);
  }
  
  return aeSUCCESS;
}

