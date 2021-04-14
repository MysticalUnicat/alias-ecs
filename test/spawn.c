#include "local.h"

struct Position {
  int x;
  int y;
};

TEST(spawn, "aeSpawn") {
  aeInstance instance;
  TEST_EQ(aeCreateInstance(NULL, &instance), aeSUCCESS, "error while setting up testing instance");

  aeComponent Position_component;
  TEST_EQ( aeRegisterComponent(instance, &(aeComponentCreateInfo) { .size = sizeof(struct Position) }, &Position_component)
         , aeSUCCESS
         , "error while setting up position component"
         );

  aeEntity entity;

  TEST_EQ(
    aeSpawn(instance, &(aeEntitySpawnInfo) {
      .layer = aeINVALID_LAYER,
      .count = 1,
      .num_components = 1,
      .components = (aeEntitySpawnComponent[]) {
        {
          .component = Position_component,
          .stride = 0,
          .data = (const void *)(struct Position[]) {
            {
              .x = 0,
              .y = 0
            }
          }
        }
      }
    }, &entity)
    , aeSUCCESS
    , "unexpected error from simple spawn"
    );

  aeDestroyInstance(instance);
}

