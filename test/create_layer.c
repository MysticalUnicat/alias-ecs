#include "local.h"

TEST(create_layer, "aeCreateLayer") {
  aeInstance instance;
  TEST_EQ(aeCreateInstance(NULL, &instance), aeSUCCESS, "error while setting up testing instance");

  aeLayer unbounded, bounded, non_destroyed;

  struct aeLayerCreateInfo ci;

  TEST_EQ(aeCreateLayer(NULL, &ci, &unbounded), aeERROR_INVALID_ARGUMENT, "expected invalid argument error when instance is NULL");
  TEST_EQ(aeCreateLayer(instance, NULL, &unbounded), aeERROR_INVALID_ARGUMENT, "expected invalid argument error when create_info is NULL");
  TEST_EQ(aeCreateLayer(instance, &ci, NULL), aeERROR_INVALID_ARGUMENT, "expected invalid argument error when layer_ptr is NULL");

  // create with unbounded entities
  ci.max_entities = 0;
  TEST_EQ(aeCreateLayer(instance, &ci, &unbounded), aeSUCCESS, "unexpected error while creating unbounded layer");

  // create limited entities
  ci.max_entities = 32;
  TEST_EQ(aeCreateLayer(instance, &ci, &bounded), aeSUCCESS, "unexpected error while creating bounded layer");

  TEST_EQ(aeDestroyLayer(instance, 0, unbounded), aeSUCCESS, "unexpected error while destroying unbounded layer");
  TEST_EQ(aeDestroyLayer(instance, 0, bounded), aeSUCCESS, "unexpected error while destroying bounded layer");

  // non-destroyed layer
  ci.max_entities = 0;
  TEST_EQ(aeCreateLayer(instance, &ci, &non_destroyed), aeSUCCESS, "unexpected error while creating non-destroyed layer");

  // expect free the memory of the the non-destroyed layer
  aeDestroyInstance(instance);
}

