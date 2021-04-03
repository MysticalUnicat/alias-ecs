#ifndef _ALIAS_ECS_TEST_LOCAL_H_
#define _ALIAS_ECS_TEST_LOCAL_H_

#include <alias/ecs.h>
#include <stdio.h>

extern struct aliasApplicationMemoryCallbacks g_stub_allocator;
extern int g_stub_allocator_fail;   ///< instruct the stub allocator to fail the next allocations
extern int g_stub_allocator_random; ///< \todo if non-zero, fail allocations if rand() < g_stub_allocator_random

struct Test {
  const char * ident;
  const char * description;
  struct Test * next;
  void (*fn)(int *);
};

extern struct Test * g_tests;

#define TEST(IDENT, DESCRIPTION)    \
  static void IDENT##_fn(int *);    \
  __attribute__((constructor))      \
  static void IDENT##_setup(void) { \
    static struct Test _test;       \
    _test = (struct Test)           \
      { .ident = #IDENT             \
      , .description = DESCRIPTION  \
      , .next = g_tests             \
      , .fn = IDENT##_fn            \
      };                            \
    g_tests = &_test;               \
  }                                 \
  static void IDENT##_fn(int * _success)

#define TEST_EQ(A, B)                                       \
  do {                                                      \
    if((A) != (B)) {                                        \
      fprintf(stderr, "TEST_EQ(" #A ", " #B ") failed!\n"); \
      *_success = 0;                                        \
      return;                                               \
    }                                                       \
  } while(0) 

#endif // _ALIAS_ECS_TEST_LOCAL_H_

