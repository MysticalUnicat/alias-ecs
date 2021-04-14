#include "local.h"

static int _compare_components( uint32_t a_num_components, const uint32_t * a_component_indexes
                              , uint32_t b_num_components, const uint32_t * b_component_indexes
                              ) {
  if(a_num_components != b_num_components) {
    return (int)a_num_components - (int)b_num_components;
  }
  if(a_num_components > 0) {
    return memcmp(a_component_indexes, b_component_indexes, sizeof(*a_component_indexes) * a_num_components);
  } else {
    return 0;
  }
}

static int _search(void * ud, const void * ap, const void * bp) {
  aeInstance instance = (aeInstance)ud;
  const struct aeComponentSet * components = (const struct aeComponentSet *)ap;
  uint32_t archetype_index = *(uint32_t *)bp;
  const struct aeArchetypeData * archetype = &instance->archetype.data[archetype_index];
  return _compare_components(components->count, components->index, archetype->components.count, archetype->components.index);
}

aeResult alias_ecs_resolve_archetype(aeInstance instance, struct aeComponentSet components, aeArchetype * out_ptr) {
  uint32_t * index_ptr = alias_ecs_bsearch( &components
                                          , instance->archetype.components_index
                                          , instance->archetype.length
                                          , sizeof(*instance->archetype.components_index)
                                          , _search
                                          , instance);

  if(index_ptr != NULL) {
    *out_ptr = *index_ptr;
    aeComponentSet_free(instance, &components);
    return aeSUCCESS;
  }

  uint32_t index = instance->archetype.length++;

  if(instance->archetype.length > instance->archetype.capacity) {
    uint32_t old_capacity = instance->archetype.capacity;
    uint32_t new_capacity = instance->archetype.length + (instance->archetype.length >> 1);
    RELOC(instance, old_capacity, new_capacity, instance->archetype.components_index);
    RELOC(instance, old_capacity, new_capacity, instance->archetype.data);
    instance->archetype.capacity = new_capacity;
  }

  *out_ptr = index;

  return aeSUCCESS;
}

