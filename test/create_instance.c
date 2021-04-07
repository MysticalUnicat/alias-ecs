#include "local.h"

TEST(create_instance, "aeCreateInstance and it's possible error conditions") {
  aeInstance instance;
  aliasApplicationMemoryCallbacks memory_callbacks = { 0 };

  TEST_EQ(aeCreateInstance(NULL, NULL), aeERROR_INVALID_ARGUMENT, "expected to require result pointer to instance");

  TEST_EQ(aeCreateInstance(&memory_callbacks, NULL), aeERROR_INVALID_ARGUMENT, "expected to require result pointer to instance");

  // default 'internal' allocator
  TEST_EQ(aeCreateInstance(NULL, &instance), aeSUCCESS, "unexpected error when creating instance (internal allocator)");
  aeDestroyInstance(instance);

  // stub allocator
  TEST_EQ(aeCreateInstance(&g_stub_allocator.cb, &instance), aeSUCCESS, "unexpected error when creating instance (test stub allocator)");
  aeDestroyInstance(instance);

  // ... then stub allocator fail
  g_stub_allocator.fail = 1;
  TEST_EQ(aeCreateInstance(&g_stub_allocator.cb, &instance), aeERROR_OUT_OF_MEMORY, "expected error when creating instance (bad allocator)");
  g_stub_allocator.fail = 0;
}

