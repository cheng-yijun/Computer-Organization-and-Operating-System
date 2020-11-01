// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "spinlock.h"

struct run {
  struct run *next;
};

struct run *alloc_list[1000000];
int alloc_size = 0;

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

extern char end[]; // first address after kernel loaded from ELF file

// Initialize free list of physical pages.
void
kinit(void)
{
  char *p;

  initlock(&kmem.lock, "kmem");
  p = (char*)PGROUNDUP((uint)end);
  for(; p + PGSIZE <= (char*)PHYSTOP; p += 2 * PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || (uint)v >= PHYSTOP) 
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(v, 1, PGSIZE);

  acquire(&kmem.lock);
  r = (struct run*)v;
  r->next = kmem.freelist;
  kmem.freelist = r;


  for (int i = 0; i < alloc_size; i++){
    if (alloc_list[i] == r){
      for (int j = i + 1; j < alloc_size; j++){
        alloc_list[j - 1] = alloc_list[j];
      }
      alloc_size--;
      break;
    }
  }
  release(&kmem.lock);
}


// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *r;
  
  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    alloc_list[alloc_size] = r;
    alloc_size += 1;
  }

  release(&kmem.lock);
  return (char*)r;
}

int 
dump_allocated(int *frames, int numframes) 
{
  if (numframes < 0 || numframes > alloc_size){
    return -1;
  }

  int index = 0;
  for (int i = alloc_size; i > alloc_size - numframes; i--){
    frames[index] = (int)alloc_list[i - 1];
    index++;
  }
  return 0;
}

