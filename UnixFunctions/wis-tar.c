#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

/*
 *This is a Linux utility to combine files into a single tar-file.
 */
int main(int argc, char** argv){
  //check the number of arguments
  if(argc < 2){
    printf("wis-tar: tar-file file [...]\n");
    exit(1);
  }
  //check the number of arguments
  if(argc < 3){
    printf("wis-tar: tar-file file [...]\n");
    exit(1);
  }
  //get the file name for tar file
  char* tarname = argv[1];
  FILE* f_tar = fopen(tarname, "w");
  //check file open
  if(f_tar == NULL){
    printf("wis-tar: cannot open file\n");
    exit(1);
  }
  //write all input files into a tar file
  for(int i = 2; i < argc; i++){
    //get the input filenames
    char* filename = argv[i];
    FILE* f = fopen(filename, "r");
    //check file opens
    if(f == NULL){
      printf("wis-tar: cannot open file\n");
      exit(1);
    }
    //check the length of the input filenames and write to tar file
    if(strlen(filename) >= 100){
      fwrite(filename, sizeof(char), 100, f_tar);
    }else {
      fwrite(filename, sizeof(char), strlen(filename), f_tar);
      for(int j = strlen(filename) + 1; j < 101; j++){
        fputc('\0', f_tar);
      }
    }
    //get the size of input file
    struct stat info;
    //check the size of the input file
    int err = stat(filename, &info);
    if(err != 0){
      printf("failed to find file size\n");
      exit(1);
    }
    //write the file size to tar file
    fwrite(&info.st_size, sizeof(long), 1, f_tar);
    //write content of input file to tar
    char ch;
    while( ( ch = fgetc(f) ) != EOF ){
     fputc(ch, f_tar);
    }
    //close files
    fclose(f);
  }

  fclose(f_tar);

  return 0;
}
