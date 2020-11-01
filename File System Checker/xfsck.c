// Author: Yijun Cheng
// Course: cs537
//instructor: Shivaram
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <limits.h>
#include <string.h>
#define stat xv6_stat  // avoid clash with host struct stat
#define dirent xv6_dirent  // avoid clash with host struct stat
#undef stat
#undef dirent

// xv6 fs img similar to vsfs
// unused | superblock | inode table | unused | bitmap (data) | data blocks

// File system super block
struct superblock {
    uint size;         // Size of file system image (blocks)
    uint nblocks;      // Number of data blocks
    uint ninodes;      // Number of inodes.
};

#define NDIRECT 12
// On-disk inode structure
struct dinode {
    short type;           // File type
    short major;          // Major device number (T_DEV only)
    short minor;          // Minor device number (T_DEV only)
    short nlink;          // Number of links to inode in file system
    uint size;            // Size of file (bytes)
    uint addrs[NDIRECT+1];   // Data block addresses
};

// INDIRECT  size
#define DIRSIZ 14
struct dirent {
    ushort inum;
    char name[DIRSIZ];
};

// defined variables
#define BSIZE 512  // block size
#define NINDIRECT   (BSIZE / sizeof(uint))
#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Special device
#define BIT (8 * sizeof(uint))
// inodes per block
#define IPB   (BSIZE / sizeof(struct dinode))
// block that contains inode i
#define IBLOCK(i)   (i / IPB + 2)
// block that contains the bit for block b
#define BITMAPBLOCK 28

int dirent_per_block = BSIZE / sizeof(struct dirent); //1 block can have how many dirents
int num_of_directory = 0;
int badsize = 0;
int exit_code = 0;
int *index_of_directory;
int *dirPointedInodes;
int *file_block_arr;
int *dir_block_arr;

void print_inode(struct dinode dip);

// FS layout:
// unused | superblock | inodes - 25 blocks | unused | bitmap | data
// 01234....2627<28>
// 1 + 1 + 25 + 1 + 1 + 995 = 1024 blocks
int main(int argc, char *argv[]) {
    int fd;
    if(argc == 2) {
        fd = open(argv[1],O_RDONLY);
    } else if(argc == 3 && strcmp(argv[1],"-r") == 0) {
        fd = open(argv[2],O_RDWR);
    }else {
        printf("Usage: program fs.img\n");
        free(dirPointedInodes);
        free(file_block_arr);
        free(dir_block_arr);
        exit(1);
    }
    if(fd < 0) {
        fprintf(stderr, "%s", "image not found.\n");
        free(dirPointedInodes);
        free(file_block_arr);
        free(dir_block_arr);
        exit(1);
    }
    struct stat sbuf;
    fstat(fd, &sbuf);
    // Create a pointer pointing to the file system
    void * img_ptr;
    if (img_ptr == (void *)-1){
        printf("fail to map\n");
        exit(1);
    }
    img_ptr = mmap(NULL,sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    struct superblock *sb = (struct superblock *) (img_ptr + BSIZE);
    struct dinode *dip = (struct dinode *) (img_ptr + 2*BSIZE);
    index_of_directory = malloc(sizeof(int) * (int)sb->ninodes);
    dirPointedInodes = malloc(sizeof(int) * (int)sb->ninodes);
    file_block_arr = malloc(sizeof(int) * (int)sb->ninodes);
    dir_block_arr = malloc(sizeof(int) * (int)sb->ninodes);
    int count_db_inode = 0;
    int directAddrArray[(int)sb->size];
    int indirectAddrArray[(int)sb->size];
    for(int i=0; i<(int)sb->size; i++) {
        directAddrArray[i] = 0;
        indirectAddrArray[i] = 0;
    }
    for(int i = 0; i < (int)sb->ninodes; i++){
        struct dinode *cur_inode =(struct dinode *)(dip + i);
        // check if the inode is valid
        if(cur_inode->type != 0 && cur_inode->type != T_FILE && cur_inode->type != T_DIR && cur_inode->type != T_DEV){
        fprintf(stderr, "%s", "ERROR: bad inode.\n");
        free(dirPointedInodes);
        free(file_block_arr);
        free(dir_block_arr);
        close(fd);
        exit(1);
        }
        //valid inode
        int used_datablock = 0;
        if(cur_inode->type != 0){ // this inode is in-used
            for(int j = 0; j < NDIRECT + 1; j++)
            {
              if(j < NDIRECT) { // in direct data blocks
                  if(cur_inode->addrs[j] == 0){
                      continue;
                  }
                  // mapped to datablocks (datablock is in used)
                  used_datablock += 1;
                  if(cur_inode->addrs[j] < (sb->size -sb->nblocks) || cur_inode->addrs[j] > sb->size - 1) {
                      fprintf(stderr, "%s", "ERROR: bad direct address in inode.\n");
                      free(dirPointedInodes);
                      free(file_block_arr);
                      free(dir_block_arr);
                      close(fd);
                      exit(1);
                  }
                  int n_shift = cur_inode->addrs[j] % BIT;
                  uint *bit_block = (uint *)(img_ptr + BSIZE * BITMAPBLOCK);
                  uint mask = bit_block[(cur_inode->addrs[j]/BIT)%(BSIZE/BIT)];
                  int this_bit = (mask >> n_shift) & 1;
                  if(this_bit != 1){
                      fprintf(stderr, "%s", "ERROR: address used by inode but marked free in bitmap.\n");
                      free(dirPointedInodes);
                      free(file_block_arr);
                      free(dir_block_arr);
                      exit(1);
                  }
                  count_db_inode++;
                  if(directAddrArray[cur_inode->addrs[j]] == 0) {
                      directAddrArray[cur_inode->addrs[j]] = 1;
                  } else {
                      fprintf(stderr, "%s", "ERROR: direct address used more than once.\n");
                      free(dirPointedInodes);
                      free(file_block_arr);
                      free(dir_block_arr);
                      close(fd);
                      exit(1);
                  }
              }
                if(j == NDIRECT)
                {
                    uint* indirectPtr = (uint *) (img_ptr + BSIZE * cur_inode->addrs[j]);
                    if(cur_inode->addrs[j]!=0)
                        count_db_inode++;
                    //printf("%d\n",cur_inode->addrs[j]);
                    if(cur_inode->addrs[j] != 0 ) {
                        if(indirectAddrArray[cur_inode->addrs[j]] == 0) {
                            indirectAddrArray[cur_inode->addrs[j]] = 1;
                        } else {
                            fprintf(stderr, "%s", "ERROR: indirect address used more than once.\n");
                            free(dirPointedInodes);
                            free(file_block_arr);
                            free(dir_block_arr);
                            close(fd);
                            exit(1);
                        }
                    }
                    for(int k = 0; k < NINDIRECT; k++)
                    {
                        if(indirectPtr[k] == 0) continue;
                        if(indirectPtr[k]<(sb->size -sb->nblocks) || indirectPtr[k] > sb->size - 1 ) {
                            fprintf(stderr, "%s", "ERROR: bad indirect address in inode.\n");
                            free(dirPointedInodes);
                            free(file_block_arr);
                            free(dir_block_arr);
                            close(fd);
                            exit(1);
                        }
                        int bBlock = BITMAPBLOCK;
                        int n_shift = indirectPtr[k] % BIT;
                        uint *bit_block = (uint *)(img_ptr + BSIZE * BITMAPBLOCK);
                        uint mask = bit_block[(indirectPtr[k]/BIT)%(BSIZE/BIT)];
                        int this_bit = (mask >> n_shift) & 1;
                        if(this_bit!= 1){
                            fprintf(stderr, "%s", "ERROR: address used by inode but marked free in bitmap.\n");
                            free(dirPointedInodes);
                            free(file_block_arr);
                            free(dir_block_arr);
                            close(fd);
                            exit(1);
                        }
                        used_datablock++;
                        count_db_inode++;
                        if(indirectAddrArray[indirectPtr[k]] == 0) {
                            indirectAddrArray[indirectPtr[k]] = 1;
                        } else {
                            fprintf(stderr, "%s", "ERROR: indirect address used more than once.\n");
                            free(dirPointedInodes);
                            free(file_block_arr);
                            free(dir_block_arr);
                            close(fd);
                            exit(1);
                        }
                    }
                }
            }
            //printf("%i\n", used_datablock);
            // int aa = 1;
            if((cur_inode->type == T_FILE) && cur_inode->size != 0 && (cur_inode->size <= (used_datablock - 1) * BSIZE|| cur_inode->size > used_datablock * BSIZE)){
                badsize = 1;
            }
        }
        // if this inode points to a directory
        // check into this directory
        if(cur_inode -> type == T_DIR) {
            int inum = i;
            int curAndpre_dir = 0;
            struct dinode *dip = (struct dinode *) (img_ptr + 2 * BSIZE);
            for(int j = 0; j < NDIRECT + 1; j++)
            {
                if(cur_inode->addrs[j] == 0) {
                  continue;
                }
                if (j < NDIRECT) { // for directly mapped data blocks
                    struct dirent * dir = (struct dirent *) (img_ptr + BSIZE * (cur_inode->addrs[j]));
                    for (int k = 0; k < dirent_per_block; k++) {
                        struct dirent * dir2 = (struct dirent *) (dir + k);
                        if(dir2->inum != 0){
                            struct dinode * ptr = (struct dinode *)(dip + dir2->inum);
                            if(ptr->type == T_FILE) {
                                file_block_arr[dir2->inum]++;
                            }else if (ptr->type == T_DIR){
                                if(strcmp(dir2->name,".") == 0 && dir2->inum != inum) {
                                    fprintf(stderr, "%s", "ERROR: directory not properly formatted.\n");
                                    free(dirPointedInodes);
                                    free(file_block_arr);
                                    free(dir_block_arr);
                                    close(fd);
                                    exit(1);
                                }
                                // check the path to root directory
                                // root inode num == 1
                                if (strcmp(dir2 -> name,"..") == 0 && dir2->inum != 1){
                                    int find_root = 0;
                                    struct dinode *copied_ptr = (struct dinode *)(dip + dir2->inum);
                                    int inode_bits[200];
                                    for (int index = 0; index < 200; index++){
                                      inode_bits[index] = 0;
                                    }
                                    inode_bits[inum] = 1;
                                    while (find_root == 0) {
                                        for (int n = 0; n < NDIRECT + 1; n++){
                                            if (n < NDIRECT){
                                                struct dirent *parent_dir = (struct dirent *) (img_ptr + BSIZE * (copied_ptr->addrs[n]));
                                                for (int mm = 0; mm < dirent_per_block; mm++){ // for each dirent
                                                    struct dirent *parent_dir2 = (struct dirent *) (parent_dir + mm);
                                                    if (strcmp(parent_dir2 -> name,"..") == 0){
                                                        if (parent_dir2->inum == 1){ // find the root
                                                            find_root = 1;
                                                            break;
                                                        }
                                                        if (inode_bits[parent_dir2->inum] == 1){ //cycle
                                                          fprintf(stderr, "%s", "ERROR: inaccessible directory exists\n");
                                                          free(dirPointedInodes);
                                                          free(file_block_arr);
                                                          free(dir_block_arr);
                                                          close(fd);
                                                          exit(1);
                                                        }
                                                        copied_ptr = (struct dinode *)(dip + parent_dir2->inum);
                                                    }
                                                }
                                            }
                                            if (n == NDIRECT){
                                                uint* indirect_parent_Ptr = (uint *) (img_ptr + BSIZE * copied_ptr->addrs[n]);
                                                for (int mm = 0; mm < NINDIRECT; mm++){
                                                    struct dirent * parent_dir = (struct dirent *) (img_ptr + BSIZE * indirect_parent_Ptr[mm]);
                                                    for (int l = 0; l < dirent_per_block; l++){
                                                        struct dirent * parent_dir2 = (struct dirent *) (parent_dir + l);
                                                        if (strcmp(parent_dir2 -> name,"..") == 0){
                                                            if (parent_dir2->inum == 1){ // find the root
                                                                find_root = 1;
                                                                break;
                                                            }
                                                            if (inode_bits[parent_dir2->inum] == 1){ //cycle
                                                              fprintf(stderr, "%s", "ERROR: inaccessible directory exists\n");
                                                              free(dirPointedInodes);
                                                              free(file_block_arr);
                                                              free(dir_block_arr);
                                                              close(fd);
                                                              exit(1);
                                                            }
                                                            copied_ptr = (struct dinode *)(dip + parent_dir2->inum);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                // check if the parent .. directory is proper
                                if(strcmp(dir2 -> name,"..") == 0){
                                    int find_back_ptr = 0;
                                    for (int n = 0; n < NDIRECT + 1; n++){ // go into the parent directory
                                        if (n < NDIRECT){
                                            struct dirent *parent_dir = (struct dirent *) (img_ptr + BSIZE * (ptr->addrs[n]));
                                            for (int m = 0; m < dirent_per_block; m++){ // for each dirent
                                                struct dirent *parent_dir2 = (struct dirent *) (parent_dir + m);
                                                if (parent_dir2->inum == inum){
                                                    find_back_ptr = 1;
                                                }
                                            }
                                        }
                                        if (n == NDIRECT){
                                            uint* indirect_parent_Ptr = (uint *) (img_ptr + BSIZE * ptr->addrs[n]);
                                            for (int m = 0; m < NINDIRECT; m++){
                                                struct dirent * parent_dir = (struct dirent *) (img_ptr + BSIZE * indirect_parent_Ptr[m]);
                                                for (int l = 0; l < dirent_per_block; l++){
                                                    struct dirent * parent_dir2 = (struct dirent *) (parent_dir + l);
                                                    if (parent_dir2->inum == inum){
                                                        find_back_ptr = 1;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    if (find_back_ptr == 0){ // cannot find the back pointer
                                      fprintf(stderr, "%s", "ERROR: parent directory mismatch\n");
                                      free(dirPointedInodes);
                                      free(file_block_arr);
                                      free(dir_block_arr);
                                      close(fd);
                                      exit(1);
                                    }
                                }

                                if(dir_block_arr[dir2->inum] >= 1 && strcmp(dir2 -> name,"..") != 0 && strcmp(dir2 -> name,".") != 0){
                                    fprintf(stderr, "%s", "ERROR: directory appears more than once in file system.\n");
                                    free(dirPointedInodes);
                                    free(file_block_arr);
                                    free(dir_block_arr);
                                    close(fd);
                                    exit(1);
                                }
                                if(strcmp(dir2 -> name,"..") != 0 && strcmp(dir2 -> name,".") != 0){
                                    dir_block_arr[dir2->inum]++;
                                }
                            }
                            if(ptr->type == 0){
                                fprintf(stderr, "%s", "ERROR: inode referred to in directory but marked free.\n");
                                free(dirPointedInodes);
                                free(file_block_arr);
                                free(dir_block_arr);
                                close(fd);
                                exit(1);
                            }

                            if(strcmp(dir2->name,".") != 0 && strcmp(dir2->name,"..")!=0){
                                dirPointedInodes[dir2->inum] ++;
                            }
                            index_of_directory[dir2->inum] = num_of_directory;
                            num_of_directory++;
                        }
                        if(strcmp(dir2->name, ".")==0 || strcmp(dir2->name, "..")==0 ) {
                            curAndpre_dir++;
                            if(strcmp(dir2->name,".") == 0 && dir2->inum != inum) { // wrong current directory
                                fprintf(stderr, "%s", "ERROR: directory not properly formatted.\n");
                                exit_code = 1;
                            }
                        }
                    }
                }
                if(j == NDIRECT){
                    uint* indirectPtr = (uint *) (img_ptr + BSIZE * cur_inode->addrs[j]);
                    for(int k = 0; k < NINDIRECT; k++)
                    {
                        struct dirent * dir = (struct dirent *) (img_ptr + BSIZE * indirectPtr[k]);
                        for (int m = 0; m < dirent_per_block; m++) {
                            struct dirent * dir2 = (struct dirent *) (dir+m);
                            if(strcmp(dir2->name, ".")==0 || strcmp(dir2->name, "..")==0 ) {
                                curAndpre_dir++;
                                if(strcmp(dir2->name,".") == 0 && dir2->inum != inum) {
                                    fprintf(stderr, "%s", "ERROR: directory not properly formatted.\n");
                                    free(dirPointedInodes);
                                    free(file_block_arr);
                                    free(dir_block_arr);
                                    close(fd);
                                    exit(1);
                                }

                            }
                            if(dir2->inum != 0){
                                struct dinode * ptr = ptr;
                                if(ptr->type == T_FILE) {
                                    file_block_arr[dir2->inum]++;
                                }else if (ptr->type == T_DIR){
                                  if(strcmp(dir2 -> name,"..") == 0){
                                      int find_back_ptr = 0;
                                      for (int n = 0; n < NDIRECT + 1; n++){ // go into the parent directory
                                          if (n < NDIRECT){
                                              struct dirent *parent_dir = (struct dirent *) (img_ptr + BSIZE * (ptr->addrs[n]));
                                              for (int mm = 0; mm < dirent_per_block; mm++){ // for each dirent
                                                  struct dirent *parent_dir2 = (struct dirent *) (parent_dir + mm);
                                                  if (parent_dir2->inum == inum){ // can find the back pointer
                                                      find_back_ptr = 1;
                                                  }
                                              }
                                          }
                                          if (n == NDIRECT){
                                              uint* indirect_parent_Ptr = (uint *) (img_ptr + BSIZE * ptr->addrs[n]);
                                              for (int mm = 0; mm < NINDIRECT; mm++){
                                                  struct dirent * parent_dir = (struct dirent *) (img_ptr + BSIZE * indirect_parent_Ptr[mm]);
                                                  for (int l = 0; l < dirent_per_block; l++){
                                                      struct dirent * parent_dir2 = (struct dirent *) (parent_dir + l);
                                                      if (parent_dir2->inum == inum){ // can find the back pointer
                                                          find_back_ptr = 1;
                                                      }
                                                  }
                                              }
                                          }
                                      }
                                      if (find_back_ptr == 0){ // cannot find the back pointer
                                        fprintf(stderr, "%s", "ERROR: parent directory mismatch\n");
                                        free(dirPointedInodes);
                                        free(file_block_arr);
                                        free(dir_block_arr);
                                        close(fd);
                                        exit(1);
                                      }
                                  }
                                  // check the path to root directory
                                  // root inode num == 1
                                  if (strcmp(dir2 -> name,"..") == 0 && dir2->inum != 1){
                                      int find_root = 0;
                                      struct dinode *copied_ptr = (struct dinode *)(dip + dir2->inum);
                                      int inode_bits[200]; // craete an array to record the number of directory
                                      for (int index = 0; index < 200; index++){
                                        inode_bits[index] = 0;
                                      }
                                      inode_bits[inum] = 1;
                                      while (find_root == 0) {
                                          for (int n = 0; n < NDIRECT + 1; n++){
                                              if (n < NDIRECT){
                                                  struct dirent *parent_dir = (struct dirent *) (img_ptr + BSIZE * (copied_ptr->addrs[n]));
                                                  for (int mm = 0; mm < dirent_per_block; mm++){ // for each dirent
                                                      struct dirent *parent_dir2 = (struct dirent *) (parent_dir + mm);
                                                      if (strcmp(parent_dir2 -> name,"..") == 0){
                                                          if (parent_dir2->inum == 1){ // find the root
                                                              find_root = 1;
                                                              break;
                                                          }
                                                          if (inode_bits[parent_dir2->inum] == 1){ //cycle
                                                            fprintf(stderr, "%s", "ERROR: inaccessible directory exists\n");
                                                            free(dirPointedInodes);
                                                            free(file_block_arr);
                                                            free(dir_block_arr);
                                                            close(fd);
                                                            exit(1);
                                                          }
                                                          // update the ptr to the next inode
                                                          copied_ptr = (struct dinode *)(dip + parent_dir2->inum);
                                                      }
                                                  }
                                              }
                                              if (n == NDIRECT){
                                                  uint* indirect_parent_Ptr = (uint *) (img_ptr + BSIZE * copied_ptr->addrs[n]);
                                                  for (int mm = 0; mm < NINDIRECT; mm++){
                                                      struct dirent * parent_dir = (struct dirent *) (img_ptr + BSIZE * indirect_parent_Ptr[mm]);
                                                      for (int l = 0; l < dirent_per_block; l++){
                                                          struct dirent * parent_dir2 = (struct dirent *) (parent_dir + l);
                                                          if (strcmp(parent_dir2 -> name,"..") == 0){
                                                              if (parent_dir2->inum == 1){ // find the root
                                                                  find_root = 1;
                                                                  break;
                                                              }
                                                              if (inode_bits[parent_dir2->inum] == 1){ //cycle
                                                                fprintf(stderr, "%s", "ERROR: inaccessible directory exists\n");
                                                                free(dirPointedInodes);
                                                                free(file_block_arr);
                                                                free(dir_block_arr);
                                                                close(fd);
                                                                exit(1);
                                                              }
                                                              // update the ptr to the next inode
                                                              copied_ptr = (struct dinode *)(dip + parent_dir2->inum);
                                                          }
                                                      }
                                                  }
                                              }
                                          }
                                      }
                                  }
                                    if(dir_block_arr[dir2->inum] >= 1 && strcmp(dir2 -> name,"..") != 0 && strcmp(dir2 -> name,".") != 0){
                                        printf("%s\n",dir2->name);
                                        fprintf(stderr, "%s", "ERROR: directory appears more than once in file system.\n");
                                        free(dirPointedInodes);
                                        free(file_block_arr);
                                        free(dir_block_arr);
                                        close(fd);
                                        exit(1);
                                    }
                                    if(strcmp(dir2 -> name,"..") != 0 && strcmp(dir2 -> name,".") != 0){
                                        dir_block_arr[dir2->inum]++;
                                    }
                                }

                                // non type of inode --> error
                                if(ptr->type == 0){
                                    fprintf(stderr, "%s", "ERROR: inode referred to in directory but marked free.\n");
                                    free(dirPointedInodes);free(file_block_arr);free(dir_block_arr);
                                    close(fd);
                                    exit(1);
                                }
                                // update the record for this directory number
                                if(strcmp(dir2->name,".") != 0 && strcmp(dir2->name,"..")!=0){
                                    dirPointedInodes[dir2->inum] ++;
                                }
                                num_of_directory ++;
                                index_of_directory[dir2->inum] = num_of_directory;
                            }
                        }
                    }
                }
            }
            if(curAndpre_dir != 2) {
                fprintf(stderr, "%s", "ERROR: directory not properly formatted.\n");
                free(dirPointedInodes);
                free(file_block_arr);
                free(dir_block_arr);
                close(fd);
                exit(1);
            }
        }
        used_datablock = 0;
    }
    int coun_db_bitmap = 0;
    uint * bitmapStart = (uint*)(img_ptr + BSIZE * BITMAPBLOCK);
    int diff = 8 * BSIZE;

    for(int i = 0; i < diff; i++) { //BIT is 32
        uint target = bitmapStart[i / BIT];
        int shift_n = i % BIT;
        int this_bit = (target >> shift_n) & 1;
        // i bit mapped start from data block
        // i bit should not excess fs size
        // record the bit (represented datablock occupied)
        if(i >= (int)sb->size - (int)sb->nblocks && i < (int)sb->size && this_bit == 1) {
            coun_db_bitmap++;
        }
    }

    //for all inodes
    for(int i = 0; i < (int)sb->ninodes; i++) {
        struct dinode* cur_inode = (struct dinode*) (dip + i);
        if(i != 0 && i != 1 && dirPointedInodes[i] < 1 && cur_inode->type != 0) {
            fprintf(stderr, "%s", "ERROR: inode marked used but not found in a directory.\n");
            free(dirPointedInodes);
            free(file_block_arr);
            free(dir_block_arr);
            exit(1);
        }
        if (cur_inode->type != 0){
            if(cur_inode->type == T_FILE && file_block_arr[i] != cur_inode->nlink) { // file hard link should be consist
                fprintf(stderr, "%s", "ERROR: bad reference count for file.\n");
                free(dirPointedInodes);
                free(file_block_arr);
                free(dir_block_arr);
                exit(1);

            }
            if (cur_inode->type == T_DIR && cur_inode->nlink != 1){ // directory hard link should be 1
                fprintf(stderr, "%s", "ERROR: directory appears more than once in file system.\n");
                free(dirPointedInodes);
                free(file_block_arr);
                free(dir_block_arr);
                close(fd);
                exit(1);
            }
        }
    }

    // check the matadata in superblock
    if(sb->size < 1022){
      fprintf(stderr, "%s", "ERROR: superblock is corrupted.\n");
      free(dirPointedInodes);
      free(file_block_arr);
      free(dir_block_arr);
      close(fd);
      exit(1);
    }

    // check if the count of datablock used in inode
    // is equal to the count of datablock used in bitmap
    if(count_db_inode < coun_db_bitmap) {
        fprintf(stderr, "%s", "ERROR: bitmap marks block in use but it is not in use.\n");
        free(dirPointedInodes);
        free(file_block_arr);
        free(dir_block_arr);
        close(fd);
        exit(1);
    }

    // file has a bad size
    if(badsize == 1){
        fprintf(stderr, "%s", "ERROR: incorrect file size in inode.\n");
        exit(1);
    }

    // exit with an error
    if (exit_code == 1){
        close(fd);
        free(file_block_arr);
        free(dir_block_arr);
        free(dirPointedInodes);
        exit(exit_code);
    }
    // free dynamically allocated fields
    close(fd);
    free(file_block_arr);
    free(dir_block_arr);
    free(dirPointedInodes);
    // exit without errors
    exit(0);
}

void print_inode(struct dinode dip) {
    printf("file type:%d,",dip.type);
    printf("nlink:%d,",dip.nlink );
    printf("size:%d,", dip.size);
    printf("first_addr:%d\n", dip.addrs[0]);
}
