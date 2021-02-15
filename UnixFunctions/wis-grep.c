#include <stdio.h>
#include <stdlib.h>
#include <string.h>

 /*
  *This is a Linux utility to find a search term line by line in the file.
  *If there is no file input, find search term in the line input by users.
  */
int main(int argc, char** argv){
  //check the number of arguments
  if(argc < 2){
    printf("wis-grep: searchterm [file …]\n");
    exit(1);
  }
  //get the search item
  char* search_term = argv[1];
  //check if there is a filename given
  if(argc < 3){//no filename given
    char *line = NULL;
    ssize_t line_size;
    size_t len = 0;
    line_size = getline(&line, &len, stdin);
    //loop to check search term
    while (line_size >= 0) {
      if(strlen(search_term) <= line_size){//possible to find search term
        for(int i = 0; i <= line_size - strlen(search_term); i++){
          int index = 0;
          int find = 1;
          for(int j = i; j < i + strlen(search_term); j++){
            if(search_term[index] != line[j]){//char does not match
              find = 0;
            }
            index++;
          }
          if(find == 1){//found the search item
            printf("%s", line);//print this line
            break;
          }
        }
      }
      line_size = getline(&line, &len, stdin);
    }
    free(line);
    line = NULL;
  }else{//has input file
    //read file one by one
    for(int i = 2; i < argc; i++){
      char* filename = argv[i];
      //open the file and read lines from the line
      FILE* f = fopen(filename, "r");
      if(f == NULL){
        printf("wis-grep: cannot open file\n");
        exit(1);
      }
      //get one line every loop
      char *line = NULL;
      size_t len = 0;
      ssize_t line_size;

      line_size = getline(&line, &len, f);

      //search and check the search item
      while (line_size >= 0){
        if(strlen(search_term) <= line_size){//possible to find search term
          for(int i = 0; i <= line_size - strlen(search_term); i++){
            int index = 0;
            int find = 1;
            for(int j = i; j < i + strlen(search_term); j++){
              if(search_term[index] != line[j]){//char does not match
                find = 0;
              }
              index++;
            }
            if(find == 1){//found the search item
              printf("%s", line);
              break;
            }
          }
        }
        //continue loop
        line_size = getline(&line, &len, f);
      }
      //free memory and close files
      free(line);
      line = NULL;
      fclose(f);
    }
  }
  return 0;
}
