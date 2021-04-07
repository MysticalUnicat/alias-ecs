#include "local.h"

#include <stdlib.h>
#include <getopt.h>
#include <string.h>

static void * _stub_malloc(void * ud, size_t size, size_t alignment) {
  (void)ud;
  (void)alignment;
  if(g_stub_allocator.fail) {
    return NULL;
  }
  void * result = malloc(size);
  g_stub_allocator.num_allocations++;
  g_stub_allocator.bytes_allocated += size;
  return result;
}

static void * _stub_realloc(void * ud, void * ptr, size_t old_size, size_t new_size, size_t alignment) {
  (void)ud;
  (void)old_size;
  (void)alignment;
  void * result = realloc(ptr, new_size);

  if(ptr == NULL) {
    g_stub_allocator.num_allocations++;
  }

  g_stub_allocator.bytes_allocated += new_size;
  g_stub_allocator.bytes_allocated -= old_size;

  return result;
}

static void _stub_free(void * ud, void * ptr, size_t size, size_t alignment) {
  (void)ud;
  (void)size;
  (void)alignment;
  free(ptr);

  g_stub_allocator.num_frees++;
  g_stub_allocator.bytes_freed += size;
}

struct StubAllocator g_stub_allocator =
  { .cb = { .malloc = _stub_malloc
          , .realloc = _stub_realloc
          , .free = _stub_free
          , .user_data = &g_stub_allocator
          }
  , .num_allocations = 0
  , .bytes_allocated = 0
  , .num_frees = 0
  , .bytes_freed = 0
  , .fail = 0
  , .random = 0
  };

struct Test * g_tests = NULL;

void _run(int * result, struct Test * test) {
  printf("Running test %s - %s\n", test->ident, test->description);
  int success = 1;

  g_stub_allocator_reset();
  
  test->fn(&success);

  if(g_stub_allocator.num_allocations != g_stub_allocator.num_frees || g_stub_allocator.bytes_allocated != g_stub_allocator.bytes_freed) {
    fprintf(stderr, "  test %s did not cleanup it's memory (or it was leaked internally)", test->ident);
    success = 0;
  }
  
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

