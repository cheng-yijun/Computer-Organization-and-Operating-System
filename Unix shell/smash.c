#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
// > 1.txt
//  >1.txt
// cmd >> i.txt

// A linked list node
struct path_Node {
    char* data;
    struct path_Node* next;
};

struct path_Node *path_ptr = NULL;
int path_length = 0;
int batchmode = 0;

/* Given a reference (pointer to pointer) to the head of a list
   and an int,  inserts a new node on the front of the list. */
void push(struct path_Node** header_ref, char* new_data){
  /* 1. allocate node */
  struct path_Node* new_node = (struct path_Node*) malloc(sizeof(struct path_Node));
  /* 2. put in the data  */
  new_node->data = new_data;
  /* 3. Make next of new node as head */
  new_node->next = (*header_ref);
  /* 4. move the head to point to the new node */
  (*header_ref) = new_node;
}

int delete_node(struct path_Node** header_ref, char* key){
  struct path_Node* temp = *header_ref;
  struct path_Node* prev = temp;
  // if key is at header
  if (temp != NULL && strcmp(temp->data, key) == 0){
    *header_ref = temp->next;
    free(temp);
    return 1;
  }

  while (temp != NULL && strcmp(temp->data, key) != 0){
    prev = temp;
    temp = temp->next;
  }

  if(temp == NULL){
    return 0;
  }
  prev->next = temp->next;
  free(temp);
  return 1;
}

void clear_path(struct path_Node** header_ref){
  struct path_Node* current = *header_ref;
  struct path_Node* next;

  while(current != NULL){
    next = current->next;
    free(current);
    current = next;
  }
  *header_ref = NULL;
}

void remove_space(char *str)
{
	char *str_c=str;
	int i,j=0;
	for(i=0;str[i]!='\0';i++)
	{
		if(str[i]!=' ')
			str_c[j++]=str[i];
	}
	str_c[j]='\0';
	str=str_c;
}

FILE* getinput(int argc, char* argv[]){
 FILE* input;
 if (argc == 1){
  batchmode = 0;
  input = stdin;
 } else {
  batchmode = 1;
  input = fopen(argv[1], "r");
  if (input == NULL){
   char error_message[30] = "An error has occurred\n";
   write(STDERR_FILENO, error_message, strlen(error_message));
   exit(1);
  }
 }
 return input;
}

char *trim(char *user_input) {
  // remove the white space in the left of the user_input
  while (isspace(*user_input)) {
    user_input++;
  }
  // remove the white space in the right of the user_input
  char *right = user_input + strlen(user_input);
  while (isspace(*--right)) {}
  // covert the char array to a string
  *(right + 1) = '\0';
  return user_input;
}

char* checkpath(char* str){
 struct path_Node* cur = path_ptr;
 for (int i = 0; i < path_length; i++){
  char* path = malloc(1024 * sizeof(char));
  if (path == NULL){
   char error_message[30] = "0An error has occurred\n";
   write(STDERR_FILENO, error_message, strlen(error_message));
   exit(1);
  }
  strcpy(path, cur->data);
  strcat(path, "/");
  strcat(path, str);
  int a = access(path, F_OK | X_OK);
  if (a == 0){
   return path;
  } else{
   cur = cur->next;
  }
 }
 return "";
}

int checksyntax(char* str){
    int error = 0;
    str = trim(str);
    char* line1 = strdup(str);
      // separate by ;
    char* p1 = strsep(&line1, ";");
    while (p1 != NULL){
      p1 = trim(p1);
      if (strlen(p1) > 0){
      char* line2 = strdup(p1);
      // separate by &
      char* p2 = strsep(&line2, "&");
      while (p2 != NULL){
       p2 = trim(p2);
       if (strlen(p2) > 0){
        char* redir = strstr(p2, ">");
        // no redirection
        if (redir == NULL){
         char* line3 = strdup(p2);
         // separate by " "
         char* p3 = strsep(&line3, " ");
         if(strcmp(p3, "exit") == 0){
           //check number of arguments for exit
           p3 = strsep(&line3, " ");
           while(p3 != NULL){
             if(strlen(p3) > 0){
               error = 1;
               return error;
             }
             p3 = strsep(&line3, " ");
           }
         }else if(strcmp(p3, "cd") == 0){
           //check number of arguments for cd
           p3 = strsep(&line3, " ");
           int count = 0;
           while(p3 != NULL){
             if(strlen(p3) > 0){
               count++;
             }
             p3 = strsep(&line3, " ");
           }
           if(count != 1){
             return 1;
           }
         }else if (strcmp(p3, "path") == 0){
           //check number of arguments for path
           p3 = strsep(&line3, " ");
           while(p3 != NULL && strlen(p3) <= 0){
             p3 = strsep(&line3, " ");
           }
           if(p3 == NULL){
             return 1;
           }
           if(strcmp(p3, "add") == 0){
             p3 = strsep(&line3, " ");
             int count = 0;
             while(p3 != NULL){
               if(strlen(p3) > 0){
                 count++;
               }
               p3 = strsep(&line3, " ");
             }
             if(count != 1){
               return 1;
             }
           }else if (strcmp(p3, "remove") == 0){
             p3 = strsep(&line3, " ");
             int count = 0;
             while(p3 != NULL){
               if(strlen(p3) > 0){
                 count++;
               }
               p3 = strsep(&line3, " ");
             }
             if(count != 1){
               return 1;
             }
           }else if(strcmp(p3, "clear") == 0){
             p3 = strsep(&line3, " ");
             while(p3 != NULL){
               if(strlen(p3) > 0){
                 return 1;
               }
               p3 = strsep(&line3, " ");
             }

           }else{
             return 1;
           }

         }else{
           char* command = checkpath(p3);
           if (strlen(command) == 0){
            error = 1;
            return error;
           }
         }

        } else{ // has redirection
         char* line3 = strdup(p2);
         // separate by ">"
         char* p3 = strsep(&line3, ">");
         char* line4 = strdup(p3);
         // separate by " "
         char* p4 = strsep(&line4, " ");
         p4 = trim(p4);
         // check path
         char* command = checkpath(p4);
         if (strlen(command) == 0){
          error = 1;
          return error;
         }
         // separate by ">"
         char* p5 = strsep(&line3, ">");
         p5 = trim(p5);
         if (strlen(p5) == 0){
          error = 1;
          return error;
         }
         // check the number of ">"
         int count = 0;
         for (int i = 0; i < strlen(p2); i++){
          if (p2[i] == '>'){
           count++;
          }
         }
         if (count > 1){
          error = 1;
          return error;
         }
        }
       }
       p2 = strsep(&line2, "&");
      }
     }
     p1 = strsep(&line1, ";");
    }
    return error;
}

int main(int argc, char** argv){

  push(&path_ptr,  "/bin");
  path_length++;

  if (argc == 1 || argc == 2){// smash mode
    FILE* user_input = getinput(argc, argv);
    while(1){
      if(batchmode == 0){
        printf("smash> ");
        fflush(stdout);
      }


      char *line = NULL;
      ssize_t line_size;
      size_t len = 0;
      line_size = getline(&line, &len, user_input);
      if(checksyntax(line) == 1){
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        continue;
      }
      if(line_size > 0 && line[line_size - 1] == '\n'){
        line_size--;
        line[line_size] = '\0';
      }
      char* cur_cmds = strsep(&line, ";");
      while(cur_cmds != NULL){

        if(strlen(cur_cmds) > 0){//get current command
          //printf("%s\n", cur_cmds);
          char *cpy_cmd= malloc(strlen(cur_cmds) + 1);//copy str of cur_cmds

          strcpy(cpy_cmd, cur_cmds);

          char* buffer_cmd = strsep(&cur_cmds, " ");

          while(buffer_cmd != NULL && strlen(buffer_cmd) <= 0){
            buffer_cmd = strsep(&cur_cmds, " ");
          }
          if(buffer_cmd == NULL){
            cur_cmds = NULL;
            cur_cmds = strsep(&line, ";");/////////////////////////
            continue;
          }

          if(strcmp(buffer_cmd, "exit") == 0){
            exit(0);
          }
          else if(strcmp(buffer_cmd, "cd") == 0){
            //printf("ssss\n");
            int valid = 1;
            char* buffer_args[2];
            int index = 0;
            while(buffer_cmd != NULL){
            //  printf("hellow its%s\n", buffer_cmd);
              if(strlen(buffer_cmd) > 0 && index == 2){
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                valid = 0;
                break;
              }
              if(strlen(buffer_cmd) > 0){
                buffer_args[index] = buffer_cmd;
                index++;
              }
              buffer_cmd = strsep(&cur_cmds, " ");
            }
            //printf("%i\n",index );
            if(index != 2){
              valid = 0;
            }
            if(valid == 1){
              int val = chdir(buffer_args[1]);
              if(val != 0){
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                break;
              }
            }else{
              char error_message[30] = "An error has occurred\n";
              write(STDERR_FILENO, error_message, strlen(error_message));
              break;
            }
            //int val = chdir();
          }
          else if(strcmp(buffer_cmd, "path") == 0){
            //printf("path\n");
            int valid = 1;// valid number of arguments for path command
            char* buffer_args[3];
            int index = 0;
            while(buffer_cmd != NULL){
              if(index == 4){

                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                valid = 0;
                break;
              }
              if(strlen(buffer_cmd) > 0){
                buffer_args[index] = buffer_cmd;
                //printf("%s\n", buffer_args[index]);
                index++;
              }
              buffer_cmd = strsep(&cur_cmds, " ");
            }
            if(valid == 1){// number of arguments is right
              if(strcmp(buffer_args[1],"add") == 0){
                push(&path_ptr,  buffer_args[2]);
                path_length++;
                //printList(path_ptr);
              }
              else if(strcmp(buffer_args[1],"remove") == 0){
                if(delete_node(&path_ptr, buffer_args[2]) == 0){
                  //error
                  char error_message[30] = "An error has occurred\n";
                  write(STDERR_FILENO, error_message, strlen(error_message));
                }else{//delete the node successfully
                  path_length--;
                }
                //printList(path_ptr);
              }
              else if(strcmp(buffer_args[1],"clear") == 0){
                clear_path(&path_ptr);
                path_length = 0;
                //printList(path_ptr);
              }
              else{//invalid arguments for path

                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
              }
            }

          }
          // command is not built in
          else{
            int success = 0;
            char* cmd = strsep(&cpy_cmd, "&");

            while(cmd != NULL){
              if(strlen(cmd) > 0){
                // remove '>' and get the real command
                char* real_cmd = strsep(&cmd, ">");
                //printf("hihihi%s\n", real_cmd);
                char* final_arg = strsep(&cmd, ">");
                char* real_arg = strsep(&final_arg, " ");
                while(real_arg != NULL && strlen(real_arg) <= 0){
                  real_arg = strsep(&final_arg, " ");
                }


                int number = 0;
                char* aa[1024];
                char* final_cmd = strsep(&real_cmd, " ");
                while(final_cmd != NULL && strlen(final_cmd) <= 0){
                  final_cmd = strsep(&real_cmd, " ");
                }//get the good command: final_cmd
                if(final_cmd == NULL){
                  cmd = strsep(&cpy_cmd, "&");
                  continue;
                }
                char *new_final_cmd = malloc(sizeof(final_cmd));
                strcpy(new_final_cmd, final_cmd);

                char* cmd_args = strsep(&real_cmd, " ");
                while(cmd_args != NULL){
                  if(strlen(cmd_args) > 0){
                    aa[number] = cmd_args;
                    number++;
                  }
                  cmd_args = strsep(&real_cmd, " ");
                }
              //  printf("78789\n");
                char* arg_list[number];
                for(int i = 0; i < number; i ++){
                  arg_list[i] = aa[i];
                  //printf("%s\n", arg_list[i]);
                }

                struct path_Node *current = path_ptr;
                for (int i = 0; i < path_length; i++){// search in the path
                  char *cpy_path= malloc(strlen(current->data) + 512);//copy str of path
                  strcpy(cpy_path, current->data);
                  strcat(cpy_path, "/");
                  strcat(cpy_path, new_final_cmd);

                  if(access(cpy_path, X_OK) == 0){
                    success = 1;
                    int rc = fork();
                    if(rc < 0){
                      char error_message[30] = "An error has occurred\n";
                      write(STDERR_FILENO, error_message, strlen(error_message));
                      exit(1);
                    }
                    else if(rc == 0){

                      int fd = open(real_arg, O_WRONLY | O_CREAT | O_TRUNC, 0600); // open file
                      int curout = dup(STDOUT_FILENO);
                      int curerr = dup(STDERR_FILENO);
                      dup2(fd, STDOUT_FILENO);
                      dup2(fd, STDERR_FILENO);
                      //char* command = checkpath(real_arg); // prepare path
                      char* myargs2[number + 2];
                      char *myargs[number + 2];
                      myargs[0] = strdup(cpy_path);
                      myargs2[0] = strdup(new_final_cmd);
                      for(int m = 0; m < number; m++){
                        myargs[m + 1] = arg_list[m];
                        myargs2[m + 1] = arg_list[m];
                      }

                      myargs2[number + 1] = NULL;

                      execv(myargs[0], myargs2);

                      dup2(curout, STDOUT_FILENO);
                      dup2(curerr, STDERR_FILENO);
                      close(fd);
                      close(curout);
                      close(curerr);
                    }
                    else{
                      int rc_wait = waitpid(rc, NULL, 0);
                      if(rc_wait == -1){
                        char error_message[30] = "An error has occurred\n";
                        write(STDERR_FILENO, error_message, strlen(error_message));

                      }
                    }
                  }

                  current = current->next;
                }
                if(success == 0){
                  char error_message[30] = "An error has occurred\n";
                  write(STDERR_FILENO, error_message, strlen(error_message));
                }
              }
              cmd = strsep(&cpy_cmd, "&");
            }

          }
        }
        wait(NULL);
        cur_cmds = strsep(&line, ";");
      }
    }
  }
}
