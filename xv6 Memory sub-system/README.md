In the ﬁrst part of project, I’ll be adding read only access to some pages in xv6, to protect the process from accidentally modifying it.
In the second part, I will implement two basic page allocation schemes. The ﬁrst scheme allocates frames alternatively to prevent malicious writes. The second scheme allocates frames randomly. I also need to implement a random number generator for second scheme. I will implement a system call
int dump_allocated(int *frames, int numframes) which dumps information about the allocated frames.
