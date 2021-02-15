#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

/*
 *This is a Linux utility to decompose a single tar-file to some output files.
 */
int main(int argc, char** argv){
  //check the number of arguments
  if(argc < 2){
    printf("wis-untar: tar-file\n");
    exit(1);
  }
  //the second argument representing the tar filename
  char* tarname = argv[1];
  FILE* f_tar = fopen(tarname, "r");
  //check the file open of tar file
  if(f_tar == NULL){
    printf("wis-untar: cannot open file\n");
    exit(1);
  }
  //read the filename of the output file from the tar file
  char filename_buffer[100];//100 bytes size
  size_t bytes;//the bytes read from the tar file
  while((bytes = fread(filename_buffer, sizeof(char),
    sizeof(filename_buffer), f_tar)) > 0){
    FILE* out_file = fopen(filename_buffer, "w");
    if(out_file == NULL){
      printf("wis-untar: cannot open file\n");
      exit(1);
    }
    //read the size of the output file from tar file
    long size_buffer;
    bytes = fread(&size_buffer, sizeof(char), sizeof(long), f_tar);
    //write the content for an output file
    char ch;
    for(int i = 0; i < size_buffer; i++){
        ch = fgetc(f_tar);
        fputc(ch, out_file);
    }
    fclose(out_file);//close files
  }

  fclose(f_tar);

  return 0;
}
