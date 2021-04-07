#include "local.h"

TEST(create_component, "aeRegisterComponent") {
  aeInstance instance;
  TEST_EQ(aeCreateInstance(NULL, &instance), aeSUCCESS, "unexpected error while creating instance");

  aeComponent component;

  TEST_EQ( aeRegisterComponent( instance
                              , &(aeComponentCreateInfo)
                                { .size = 0
                                , .num_required_components = 0
                                , .required_components = NULL
                              }
                              , &component
                              )
         , aeERROR_INVALID_ARGUMENT
         , "expected invalid argument (size is zero)"
         );

  TEST_EQ( aeRegisterComponent( instance
                              , &(aeComponentCreateInfo)
                                { .size = sizeof(uint32_t)
                                , .num_required_components = 1
                                , .required_components = NULL
                              }
                              , &component
                              )
         , aeERROR_INVALID_ARGUMENT
         , "expected invalid argument (required_components is NULL while num_required_components is greater than zero)"
         );

  component = 0;
  TEST_EQ( aeRegisterComponent( instance
                              , &(aeComponentCreateInfo)
                                { .size = sizeof(uint32_t)
                                , .num_required_components = 1
                                , .required_components = &component
                              }
                              , &component
                              )
         , aeERROR_INVALID_ARGUMENT
         , "expected invalid argument (no components have been successfully registerered, component ids start at zero so even zero should be invalid)"
         );

  TEST_EQ( aeRegisterComponent( instance
                              , &(aeComponentCreateInfo)
                                { .size = sizeof(uint32_t)
                                , .num_required_components = 0
                                , .required_components = NULL
                              }
                              , &component
                              )
         , aeSUCCESS
         , "unexpected error while creating basic 4 byte component"
         );

  TEST_EQ( aeRegisterComponent( instance
                              , &(aeComponentCreateInfo)
                                { .size = sizeof(uint32_t)
                                , .num_required_components = 1
                                , .required_components = &component
                              }
                              , &component
                              )
         , aeSUCCESS
         , "unexpected error while creating basic 4 byte component with required component"
         );

  aeDestroyInstance(instance);
}
