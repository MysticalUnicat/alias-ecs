#include "local.h"

TEST(spawn, "aeSpawn") {
  aeInstance instance;
  TEST_EQ(aeCreateInstance(NULL, &instance), aeSUCCESS, "error while setting up testing instance");
  
  aeDestroyInstance(instance);
}

