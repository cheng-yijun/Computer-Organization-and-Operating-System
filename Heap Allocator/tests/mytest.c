#include <assert.h>
#include <stdlib.h>
#include "heapAlloc.h"

int main() {
  assert(initHeap(4096) == 0);
  void* ptr[5];

  ptr[0] = allocHeap(796);//800
  ptr[1] = allocHeap(796);//800
  ptr[2] = allocHeap(1596);//1600
  ptr[3] = allocHeap(796);//800
  ptr[4] = allocHeap(84);//88

  assert(freeHeap(ptr[1]) == 0);
  assert(freeHeap(ptr[3]) == 0);

  ptr[1] = allocHeap(233);//240
  ptr[3] = allocHeap(797);//800

  assert(ptr[0] != NULL);
  assert(ptr[1] != NULL);
  assert(ptr[2] != NULL);
  assert(ptr[3] == NULL);
  assert(ptr[4] != NULL);
}
