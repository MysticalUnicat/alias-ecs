#include "local.h"

#include <stdlib.h>
#include <getopt.h>
#include <string.h>

int g_stub_allocator_fail = 0;

static void * _stub_malloc(void * ud, size_t size, size_t alignment) {
  (void)ud;
  (void)alignment;
  if(g_stub_allocator_fail) {
    return NULL;
  }
  return malloc(size);
}

static void * _stub_realloc(void * ud, void * ptr, size_t old_size, size_t new_size, size_t alignment) {
  (void)ud;
  (void)old_size;
  (void)alignment;
  return realloc(ptr, new_size);
}

static void _stub_free(void * ud, void * ptr, size_t alignment) {
  (void)ud;
  (void)alignment;
  free(ptr);
}

struct aliasApplicationMemoryCallbacks g_stub_allocator =
  { .malloc = _stub_malloc
  , .realloc = _stub_realloc
  , .free = _stub_free
  , .user_data = NULL
  };

struct Test * g_tests = NULL;

void _run(int * result, struct Test * test) {
  printf("Running test %s - %s\n", test->ident, test->description);
  int success = 1;
  test->fn(&success);
  if(success == 0) {
    *result = 1;
  }
}

void _run_by_ident(int * result, const char * ident) {
  struct Test * test = g_tests;
  while(test != NULL) {
    if(strcmp(test->ident, ident) == 0) {
      _run(result, test);
      return;
    }
    test = test->next;
  }
  fprintf(stderr, "Test %s not found\n", ident);
  *result = 1;
}

void _run_all(int * result, int fail_fast) {
  struct Test * test = g_tests;
  while(test != NULL) {
    _run(result, test);
    if(*result && fail_fast) {
      return;
    }
    test = test->next;
  }
}

int main(int argc, char * argv []) {
  int fail_fast = 0;
  
  struct option long_options[] = {
    {"fail-fast",       no_argument, &fail_fast,   1},
    {     "test", required_argument,          0, 't'},
    {          0,                 0,          0,   0}
  };

  int result = 0;
  int ran_tests = 0;

  for(;;) {
    int option_index = 0;
    int c = getopt_long(argc, argv, "f", long_options, &option_index);
    if(c == -1) {
      break;
    }
    switch(c) {
    case 0:
      if(long_options[option_index].flag != NULL) {
        break;
      }
      break;
    case 't':
      _run_by_ident(&result, optarg);
      ran_tests = 1;
      break;
    }
  }

  if(!ran_tests) {
    _run_all(&result, fail_fast);
  }

  return result;
}

