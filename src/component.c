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
    return_if_ERROR( alias_ecs_malloc( instance
                                     , sizeof(*required_components) * create_info->num_required_components
                                     , alignof(*required_components)
                                     , (void **)&required_components
                                     ));
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
