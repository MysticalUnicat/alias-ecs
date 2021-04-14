#include "local.h"

#include "../src/local.h"

TEST(component_set, "test for internal aeComponentSet") {
  aeInstance instance;
  aeCreateInstance(&g_stub_allocator.cb, &instance);
  
  struct aeComponentSet set_singleton, set_all;

  aeComponent fake_component_1 = 0;
  aeComponent fake_component_2 = 1;
  aeComponent fake_component_3 = 2;

  TEST_EQ(aeComponentSet_init(instance, &set_singleton, 1, &fake_component_1), aeSUCCESS, "failed creating one component component set");

  TEST_EQ(aeComponentSet_init( instance
                             , &set_all
                             , 3
                             , (aeComponent[])
                               { fake_component_1
                               , fake_component_3
                               , fake_component_2
                               }
                             )
         , aeSUCCESS
         , "failed creating one component component set");

  aeComponentSet_free(instance, &set_singleton);
  aeComponentSet_free(instance, &set_all);

  aeDestroyInstance(instance);
}
