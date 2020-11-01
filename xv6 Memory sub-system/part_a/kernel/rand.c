#include "rand.h"

struct xorshift32_state {
  unsigned int a;
};

struct xorshift32_state state = {1}; //initialize state.a to 1 by default


int xv6_rand (void){
    unsigned int x = state.a;
    x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
    state.a = x % ((unsigned int)XV6_RAND_MAX + 1);
    return state.a;
}

void xv6_srand (unsigned int seed){
    state.a = seed;
}