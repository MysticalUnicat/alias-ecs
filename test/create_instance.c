#include "local.h"

TEST(create_instance, "aeCreateInstance and it's possible error conditions") {
  aeInstance instance;
  aliasApplicationMemoryCallbacks memory_callbacks = { 0 };

  TEST_EQ(aeCreateInstance(NULL, NULL), aeERROR_INVALID_ARGUMENT);

  TEST_EQ(aeCreateInstance(&memory_callbacks, NULL), aeERROR_INVALID_ARGUMENT);

  // default 'internal' allocator
  TEST_EQ(aeCreateInstance(NULL, &instance), aeSUCCESS);
  aeDestroyInstance(instance);

  // stub allocator
  TEST_EQ(aeCreateInstance(&g_stub_allocator, &instance), aeSUCCESS);
  aeDestroyInstance(instance);

  // ... then stub allocator fail
  g_stub_allocator_fail = 1;
  TEST_EQ(aeCreateInstance(&g_stub_allocator, &instance), aeERROR_OUT_OF_MEMORY);
  g_stub_allocator_fail = 0;
}

