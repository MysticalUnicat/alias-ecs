#ifndef _ALIAS_ECS_H_
#define _ALIAS_ECS_H_

#include <stdint.h>
#include <stddef.h>

/// @defgroup alias Alias
/// @{
#ifndef _ALIAS_APPLICATION_MEMORY_CALLBACKS
#define _ALIAS_APPLICATION_MEMORY_CALLBACKS

/** A structure holding user provided functions for memory management. */
typedef struct aliasApplicationMemoryCallbacks {
  /** application-defined memory allocation function */
  void * (* malloc)(void * user_data, size_t size, size_t alignment);

  /** application-defined memory reallocation function */
  void * (* realloc)(void * user_data, void * ptr, size_t old_size, size_t new_size, size_t alignment);

  /** application-defined memory free function */
  void (* free)(void * user_data, void * ptr, size_t alignment);

  /** a value to be iterpreted by the allocation and sent to each function */
  void * user_data;
} aliasApplicationMemoryCallbacks;

#endif // _ALIAS_APPLICATION_MEMORY_CALLBACKS
/// @}

/// @defgroup ecs Alias-ECS
/// @{
#define ALIAS_ECS_VERSION_MAJOR 0 ///< Alias-ECS version major
#define ALIAS_ECS_VERSION_MINOR 1 ///< Alias-ECS version minor
#define ALIAS_ECS_VERSION_PATCH 0 ///< Alias-ECS version patch

/** success and error codes returned by all functions that can fail
 */
typedef enum aeResult {
  aeSUCCESS = 0,                 ///< command successfully completed
  aeERROR_INVALID_ARGUMENT = -1, ///< an argument given to the command is invalid
  aeERROR_OUT_OF_MEMORY = -2,    ///< a memory allocation has failed
  aeERROR_DOES_NOT_EXIST = -3    ///< a requested object does not exist
} aeResult;
/// @}

/// @defgroup instance Alias-ECS Instance
/// @{
/** opaque pointer to an Alias ECS Instance
 */
typedef struct aeInstance * aeInstance;

/** Create an Alias-ECS instance.
 * 
 * @param[in] memory_callbacks
 *            may be NULL
 * 
 * @param[out] instance_ptr
 *             writes the newly allocated instance
 * 
 * @return
 *  - aeSUCCESS the instance was created and initialized successfully
 *  - aeERROR_INVALID_ARGUMENT memory_callbacks is not NULL but a function pointer within it was NULL
 *  - aeERROR_INVALID_ARGUMENT instance_ptr is NULL
 *  - aeERROR_OUT_OF_MEMORY a memory allocation failed
 */
aeResult aeCreateInstance(const aliasApplicationMemoryCallbacks * memory_callbacks, aeInstance * instance_ptr);

/** Destructor for an Instance
 * 
 * frees all memory allocated by this instance and it-self.
 * 
 * @param[in] instance
 *            The instance to destory
 */
void aeDestroyInstance(aeInstance instance);
/// @}

/// @defgroup layer Alias-ECS Layer
/// @{
/** layers are represented by a single 32-bit integer */
typedef uint32_t aeLayer;

/** a value indicating an invalid layer value */
#define aeINVALID_LAYER 0xFFFFFFFF

/** data for creating a layer */
typedef struct aeLayerCreateInfo {
  /** if this is non-zero, indicate the number of entities and max entities to allocate memory for */
  uint32_t max_entities;
} aeLayerCreateInfo;

/** Create a Alias-ECS Layer
 *
 * @param[in] instance
 *            instance to create the layer for
 *
 * @param[in] create_info
 * 
 * @param[out] layer_ptr
 *             writes a value to be used to reference the created layer
 * 
 * @return
 *  - aeSUCCESS the layer was created
 *  - aeERROR_OUT_OF_MEMORY unable to allocate memory for this layer
 */
aeResult aeCreateLayer(aeInstance instance, const aeLayerCreateInfo * create_info, aeLayer * layer_ptr);

/** Destructor for an Layer
 *
 * This will destroy all entities remaining in the layer as well
 * 
 * @param[in] instance
 *            instance that the layer was created on
 * 
 * @param[in] layer
 *            the layer to destroy
 */
void aeDestroyLayer(aeInstance instance, aeLayer layer);
/// @}

/// @defgroup component Alias-ECS Component
/// @{
/** components are represented by a single 32-bit integer */
typedef uint32_t aeComponent;

/** creation data for a component */
typedef struct aeComponentCreateInfo {
  /** size of the data structure for this component */
  size_t size;

  /** number of required components */
  uint32_t num_required_components;

  /** pointer to memory of `num_required_components` aeComponents */
  const aeComponent * required_components;
} aeComponentCreateInfo;
  
/** register a component with the instance
 *
 * @param[in] instance
 *            the instance to create register the component on
 * 
 * @param[in] create_info
 *            pointer to aeComponentCreateInfo; primary source of arguments for the function
 * 
 * @param[out] component_ptr
 *             writes a value to be used to reference the created component
 * 
 * @return
 *  - aeSUCCESS
 *  - aeERROR_INVALID_ARGUMENT 
 */
aeResult aeRegisterComponent(aeInstance instance, const aeComponentCreateInfo * create_info, aeComponent * component_ptr);
/// @}

/// @defgroup entity Alias-ECS Entity
/// @{
/** entities are represented by a single 64-bit integer */
typedef uint64_t aeEntity;

/** this structure is used exclusivly by aeEntitySpawnInfo */
typedef struct aeEntitySpawnComponent {
  /** the component this data represents */
  aeComponent component;

  /** the stride of the memory pointed to by data
   * 
   * if non-zero, the stride that the data will be copied into the component data of the new entities.
   * if zero the stride is the same as the size of the component
   */
  uint32_t stride;

  /** pointer to the data to be copied into the spawned entities.
   * 
   * must be component size x aeEntitySpawnInfo::count in size at minimum
   */
  const void * data;
} aeEntitySpawnComponent;

/** used for aeSpawn */
typedef struct aeEntitySpawnInfo {
  /** the layer the entities will spawn into. can be aeINVALID_LAYER */
  aeLayer layer;

  /** the number of entities to spawn */
  uint32_t count;

  /** the number of components per entity */
  uint32_t num_components;

  /** pointer to num_components component data */
  const aeEntitySpawnComponent * components;
} aeEntitySpawnInfo;

/** spawn entities
 * 
 * Spawn's entities using the data provided
 * 
 * @param[in] instance
 *            the instance to create the entities in
 * 
 * @param[in] spawn_info
 *            data describing the components, layer, and data of the entities spawned
 * 
 * @param[out] entities_ptr
 *             if success, writes the entities into this memory
 * 
 * @return
 *  - aeSUCCESS                all entities spawned
 *  - aeERROR_DOES_NOT_EXIST   a component or layer specified does not exist
 *  - aeERROR_OUT_OF_MEMORY    ...
 *  - aeERROR_INVALID_ARGUMENT instance is NULL
 */
aeResult aeSpawn(aeInstance instance, const aeEntitySpawnInfo * spawn_info, aeEntity * entities_ptr);

/** add data to an entity's component
 */
aeResult aeAddEntityComponent(aeInstance instance, aeEntity entity, aeComponent component, const void * data);

/** remove data from an entitiy */
aeResult aeRemoveEntityComponent(aeInstance instance, aeEntity entity, aeComponent component);

/** get a writable pointer to component data for an entity */
aeResult aeWriteEntityComponent(aeInstance instance, aeEntity entity, aeComponent component, void ** out_ptr);

/** get a read-only pointer to component data for an entity */
aeResult aeReadEntityComponent(aeInstance instance, aeEntity entity, aeComponent component, const void ** out_ptr);

/** kill an entity, removes it globally */
aeResult aeDespawn(aeInstance instance, uint32_t num_entities, const aeEntity * entities);
/// @}

/// @defgroup query Alias-ECS Query
/// @{
 
/// @}

#endif // _ALIAS_ECS_H_

