#include "local.h"

aeResult aeRegisterComponent(aeInstance instance, const aeComponentCreateInfo * create_info, aeComponent * component_ptr) {
  return_ERROR_INVALID_ARGUMENT_if(instance == NULL);
  return_ERROR_INVALID_ARGUMENT_if(create_info == NULL);
  return_ERROR_INVALID_ARGUMENT_if(component_ptr == NULL);
  return_ERROR_INVALID_ARGUMENT_if(create_info->size == 0);
  return_ERROR_INVALID_ARGUMENT_if(create_info->num_required_components > 0 && create_info->required_components == NULL);

  for(uint32_t i = 0; i < create_info->num_required_components; i++) {
    return_ERROR_INVALID_ARGUMENT_if(create_info->required_components[i] >= instance->component.length);
  }

  aeComponent * required_components = NULL;

  if(create_info->num_required_components > 0) {
    ALLOC(instance, create_info->num_required_components, required_components);
    memcpy(required_components, create_info->required_components, create_info->num_required_components * sizeof(*required_components));
  }

  struct aeComponentData component_data =
    { .size = create_info->size
    , .num_required_components = create_info->num_required_components
    , .required_components = required_components
  };

  return_if_ERROR(Vector_space_for(instance, &instance->component, 1));

  *component_ptr = instance->component.length;

  *Vector_push(&instance->component) = component_data;

  return aeSUCCESS;
}

static int _compar_component_index(const void * ap, const void * bp) {
  uint32_t a = *(uint32_t *)ap;
  uint32_t b = *(uint32_t *)bp;
  return a - b;
}

aeResult _aeComponentSet_init( aeInstance instance, struct aeComponentSet * set
                             , uint32_t a_count, const uint32_t * a_component_indexes
                             , uint32_t b_count, const uint32_t * b_component_indexes
                             ) {
  set->count = a_count + b_count;
  set->index = NULL;
  if(set->count == 0) {
    return aeSUCCESS;
  }
  ALLOC(instance, set->count, set->index);
  if(a_count > 0) {
    memcpy(set->index, a_component_indexes, a_count * sizeof(*set->index));
  }
  if(b_count > 0) {
    memcpy(set->index + a_count, b_component_indexes, b_count * sizeof(*set->index));
  }
  qsort(set->index, set->count, sizeof(*set->index), _compar_component_index);
  return aeSUCCESS;
}

aeResult aeComponentSet_init(aeInstance instance, struct aeComponentSet * set, uint32_t count, const aeComponent * components) {
  // if aeComponent ever changes, need to translate to the indexes
  return _aeComponentSet_init(instance, set, count, components, 0, NULL);
}

aeResult aeComponentSet_add(aeInstance instance, struct aeComponentSet * dst, const struct aeComponentSet * src, aeComponent component) {
  // if aeComponent ever changes, need to translate to the indexes
  return _aeComponentSet_init(instance, dst, src->count, src->index, 1, &component);
}

aeResult aeComponentset_remove(aeInstance instance, struct aeComponentSet * dst, const struct aeComponentSet * src, aeComponent component) {
  // if aeComponent ever changes, need to translate to the indexes
  uint32_t order = aeComponentSet_order_of(src, component);
  if(order == UINT32_MAX) {
    return _aeComponentSet_init(instance, dst, src->count, src->index, 0, NULL);
  } else {
    return _aeComponentSet_init(instance, dst, order, src->index, src->count - order - 1, src->index + order + 1);
  }
}

uint32_t aeComponentSet_order_of(const struct aeComponentSet * set, aeComponent component) {
  // if aeComponent ever changes, need to translate to the indexes
  uint32_t * p = bsearch(&component, set->index, set->count, sizeof(*set->index), _compar_component_index);
  if(p == NULL) {
    return UINT32_MAX;
  }
  return (uint32_t)(p - set->index);
}

int aeComponentSet_contains(const struct aeComponentSet * set, aeComponent component) {
  return aeComponentSet_order_of(set, component) != UINT32_MAX;
}

int aeComponentSet_is_subset(const struct aeComponentSet * set, const struct aeComponentSet * subset) {
  for(uint32_t s = 0, ss = 0; ss < subset->count; ++ss) {
    while(s < set->count && set->index[s] < subset->index[ss]) s++;
    if(s >= set->count) {
      return 0;
    }
    if(subset->index[ss] != set->index[s]) {
      return 0;
    }
  }
  return 1;
}

void aeComponentSet_free(aeInstance instance, struct aeComponentSet * set) {
  if(set->index != NULL) {
    FREE(instance, set->count, set->index);
  }
}
