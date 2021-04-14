#include "local.h"

#define MAX_ENTITIES_FOR_LINEAR_OPS 512 // number decided by dice roll

aeResult alias_ecs_layer_validate(const aeInstance instance, aeLayer layer, uint32_t * index_ptr) {
  uint32_t index = (uint32_t)(layer & 0xFFFFFFFF);
  uint32_t generation = (uint32_t)(layer >> 32);
  if(index >= instance->layer.length) {
    return aeERROR_INVALID_LAYER;
  }
  if(generation != instance->layer.generation[index]) {
    return aeERROR_INVALID_LAYER;
  }
  *index_ptr = index;
  return aeSUCCESS;
}

static int _compare_entity_index(const void * ap, const void * bp) {
  uint32_t a = *(uint32_t *)ap;
  uint32_t b = *(uint32_t *)bp;
  return a - b;
}

static void _linear_remove_entity(struct aeLayerData * data, uint32_t entity_index) {
  for(uint32_t i = 0; i < data->entities.length; i++) {
    if(data->entities.data[i] == entity_index) {
      Vector_remove_at(&data->entities, i);
      return;
    }
  }
  // the case where the entity is not found should not happen
  // TODO: record such case
}

void alias_ecs_unset_layer(aeInstance instance, uint32_t entity_index) {
  uint32_t old_layer_index = instance->entity.layer_index[entity_index];
  if(old_layer_index == 0) {
    return;
  }
  instance->entity.layer_index[entity_index] = 0;

  struct aeLayerData * data = &instance->layer.data[old_layer_index];

  if(data->dirty && data->entities.length < MAX_ENTITIES_FOR_LINEAR_OPS) {
    _linear_remove_entity(data, entity_index);
    return;
  }

  if(data->dirty) {
    Vector_qsort(&data->entities, _compare_entity_index);
  }

  // the case where the entity is not found should not happen
  // TODO: record such case
  uint32_t * index_ptr = Vector_bsearch(&data->entities, _compare_entity_index, &entity_index);
  if(index_ptr != NULL) {
    if(data->entities.length < MAX_ENTITIES_FOR_LINEAR_OPS) {
      Vector_remove_at(&data->entities, index_ptr - data->entities.data);
    } else {
      Vector_swap_pop(&data->entities, *index_ptr);
      data->dirty = 1;
    }
  }
}

aeResult alias_ecs_set_layer(aeInstance instance, uint32_t entity_index, uint32_t layer_index) {
  if(layer_index == 0) {
    return aeSUCCESS;
  }

  struct aeLayerData * data = &instance->layer.data[layer_index];

  if(data->at_max && data->entities.length >= data->entities.capacity) {
    return aeERROR_INVALID_ENTITY;
  }

  instance->entity.layer_index[entity_index] = layer_index;

  return_if_ERROR(Vector_space_for(instance, &instance->layer.data[layer_index].entities, entity_index));
  *Vector_pop(&instance->layer.data[layer_index].entities) = entity_index;

  return aeSUCCESS;
}

aeResult aeCreateLayer(aeInstance instance, const aeLayerCreateInfo * create_info, aeLayer * layer_ptr) {
  uint32_t index;

  return_ERROR_INVALID_ARGUMENT_if(instance == NULL);
  return_ERROR_INVALID_ARGUMENT_if(create_info == NULL);
  return_ERROR_INVALID_ARGUMENT_if(layer_ptr == NULL);

  if(instance->layer.free_indexes.length > 0) {
    index = *Vector_pop(&instance->layer.free_indexes);
  } else {
    index = instance->layer.length++;
  }

  if(instance->layer.length > instance->layer.capacity) {
    uint32_t old_capacity = instance->layer.capacity;
    uint32_t new_capacity = instance->layer.length + (instance->layer.length >> 1);
    RELOC(instance, old_capacity, new_capacity, instance->layer.generation);
    RELOC(instance, old_capacity, new_capacity, instance->layer.data);
    instance->layer.capacity = new_capacity;
  }
 
  struct aeLayerData * data = instance->layer.data + index;

  if(create_info->max_entities > 0) {
    data->at_max = 1;
    Vector_set_capacity(instance, &data->entities, create_info->max_entities);
  }

  uint32_t generation = instance->layer.generation[index];

  *layer_ptr = ((uint64_t)generation << 32) | (uint64_t)index;

  return aeSUCCESS;
}

aeResult aeDestroyLayer(aeInstance instance, uint32_t unlink_entities, aeLayer layer) {
  uint32_t layer_index;
  return_if_ERROR(alias_ecs_layer_validate(instance, layer, &layer_index));
  struct aeLayerData * data = &instance->layer.data[layer_index];
  for(uint32_t i = 0; i < data->entities.length; i++) {
    if(unlink_entities) {
      alias_ecs_set_layer(instance, data->entities.data[i], 0);
    }
  }
  return aeSUCCESS;
}

