#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"

int
main(int argc, char *argv[])
{
    struct pstat st;

    if(argc != 2){
        printf(1, "usage: mytest counter");
        exit();
    }

    int i, x, l, j;
    int mypid = getpid();

    for(i = 1; i < atoi(argv[1]); i++){
        x = x + i;
    }

    getprocinfo(&st);
    for (j = 0; j < NPROC; j++) {
        if (st.inuse[j] && st.pid[j] >= 3 && st.pid[j] == mypid) {
            for (l = 3; l >= 0; l--) {
                printf(1, "level:%d \t ticks-used:%d and wait ticks is : %d\n", l, st.ticks[j][l], st.wait_ticks[j+1][3]);
            }
        }
        // printf(2, "pid is :%d \n", st.pid[j]);
        // for (l = 3; l >= 0; l--) {
        //     printf(3,"wait ticks at level %d is : %d\n ", l , st.wait_ticks[j][l]);
        // }
        
        //printf(4, "\n");
    }
    
    exit();
    return 0;
}