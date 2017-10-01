#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
/**********************************************GLOBAL VARIABLES***********************************************/
int pipe_count=0, fd,flag, len;
static char* args[512];
char input_buffer[1024];
char *cmd_exec[100],*history_file;;
char cwd[1024];
int flag_pipe=1;
pid_t pid;
int environmment_flag,no_of_lines;;
int flag_pipe, flag_without_pipe,  output_redirection, input_redirection,pid, status,bang_flag;
char current_directory[1000],history_data[1000][1000];
char *input_redirection_file,*output_redirection_file;
char ret_file[3000],his_var[2000];
extern char** environ;
/**********************************************Header Files Used**********************************************/
void clear_variables();
void fileprocess ();
void filewrite();
void bang_execute();
void environmment();
void set_environment_variables();
void change_directory();
void parent_directory();
void echo_calling(char *echo_val);
void history_execute_with_constants();
void issue_command();
int rmexcept(char **args);
static char* skipwhite(char* s);
void tokenise_commands(char *com_exec);
void tokenise_redirect_input_output(char *cmd_exec);
void tokenise_redirect_input(char *cmd_exec);
void tokenise_redirect_output(char *cmd_exec);
char* skipcomma(char* str);
static int split(char *cmd_exec, int, int, int);
void with_pipe_execute();
static int command(int, int, int, char *cmd_exec);
void prompt();
void sigintHandler(int sig_num);

/*************************************************************************************************************/
void sigintHandler(int sig_num) {
    signal(SIGINT, sigintHandler);
    fflush(stdout);
}
void clear_variables() {
  fd=pid=flag=len=no_of_lines=pipe_count=flag_pipe=flag_without_pipe=output_redirection=input_redirection=0;
  input_buffer[0]=cwd[0] = '\0';
  environmment_flag=bang_flag=0;
}

void fileprocess () {
  int fd;
  history_file=(char *)malloc(100*sizeof(char));
  strcpy(history_file,current_directory);
  strcat(history_file, "/");
  strcat(history_file, "history.txt"); 
  fd=open(history_file, O_RDONLY|O_CREAT,S_IRUSR|S_IWUSR);
  int bytes_read=0, i=0, x=0, index=0;
  char buffer[1], temp_data[1000];
    do {
        bytes_read = read (fd, buffer, sizeof (buffer));
        for (i=0; i<bytes_read; ++i) {
            temp_data[index]=buffer[i];
            index++;
            if(buffer[i]=='\n') {
                temp_data[index-1]='\0';
                no_of_lines++;
                index=0;
                strcpy(history_data[x], temp_data);
                x++;
                temp_data[0]='\0';
             }
         }
    }
    while (bytes_read == sizeof (buffer)); 
    close (fd); 
}
void filewrite(){
  int fd_out,ret_write,str_len=0;
  char input_data[2000];
  no_of_lines++;
  char no[10];
  sprintf (no, "%d", no_of_lines ); 
  strcpy(input_data, " ");
  strcat(input_data, no);
  strcat(input_data, " ");
  strcat(input_data, input_buffer);
  str_len = strlen(input_data);
  fd_out=open(history_file,O_WRONLY|O_APPEND|O_CREAT,S_IRUSR|S_IWUSR);
  len=strlen(input_buffer);
  ret_write=write(fd_out,input_data,str_len);
  if(ret_write<0) {
      printf("Error in writing file\n");
      return;
  }
  close(fd_out);
}
void bang_execute() {
  char bang_val[1000];
  char *tokenise_bang[100], *num_ch[10];
  int i, n=1, num, index=0;
  i=1;
  if(input_buffer[i]=='!') {
           strcpy(bang_val, history_data[no_of_lines-1]);
  }
  else if(input_buffer[i]=='-') {
        n=1;
        num_ch[0]=strtok(input_buffer,"-");
        while ((num_ch[n]=strtok(NULL,"-"))!=NULL)
              n++;
        num_ch[n]=NULL;
        num = atoi(num_ch[1]);
        index = no_of_lines-num;
        strcpy(bang_val, history_data[index]);
  }
  else {
      num_ch[0]=strtok(input_buffer,"!");
      num = atoi(num_ch[0]);
      strcpy(bang_val, history_data[num-1]);
  }
  tokenise_bang[0]=strtok(bang_val," ");
  while ((tokenise_bang[n]=strtok(NULL,""))!=NULL)
      n++;
  tokenise_bang[n]=NULL;
  strcpy(bang_val, tokenise_bang[1]);
  printf("%s\n", bang_val );
  strcpy(input_buffer, bang_val);
}

void environmment() {
  int i =1, index=0;
  char env_val[1000], *value;
  while(args[1][i]!='\0') {
      env_val[index]=args[1][i];
      index++;
      i++;
  }
  env_val[index]='\0';
  value=getenv(env_val);
  if(!value)
      printf("\n");
  else printf("%s\n", value);
}

void set_environment_variables() {
    int n=1;
    char *left_right[100];
    if(args[1]==NULL) {
        char** env;
          for (env = environ; *env != 0; env++) {
            char* value = *env;
            printf("declare -x %s\n", value);    
          }  
         return; 
      }
    left_right[0]=strtok(args[1],"=");
    while ((left_right[n]=strtok(NULL,"="))!=NULL)
        n++;
    left_right[n]=NULL;
    setenv(left_right[0], left_right[1], 0);
}

void change_directory() {
    char *h="/home";
    if(args[1]==NULL)
        chdir(h);
    else if ((strcmp(args[1], "~")==0) || (strcmp(args[1], "~/")==0))
        chdir(h);
    else if(chdir(args[1])<0)
        printf("bash: cd: %s: No such file or directory\n", args[1]);
}

void parent_directory() {
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
         printf("%s\n", cwd );
    }
    else
       perror("getcwd() error");
}
void echo_calling(char *echo_val) {
  int i=0, index=0;
  environmment_flag=0;
  char new_args[1024],env_val[1000], *str[10];
  str[0]=strtok(echo_val," ");
  str[1]=strtok(NULL, "");
  strcpy(env_val, args[1]);
  if(str[1]==NULL) {
          printf("\n");
          return;
  }
  if (strchr(str[1], '$')) 
      environmment_flag=1;
  memset(new_args, '\0', sizeof(new_args));
  i=0; 
  while(str[1][i]!='\0') {
      if(str[1][i]=='"') {
          index=0;
          while(str[1][i]!='\0'){
              if(str[1][i]!='"') {
                  new_args[index]=str[1][i];
                  flag=1;
                 index++;
               }
               i++;
          }             
      }
      else if(str[1][i]==39) {
          index=0;
          while(str[1][i]!='\0') {
              if(str[1][i]!=39) {
                 new_args[index]=str[1][i];
                 flag=1;
                 index++;
               }
               i++;
           }
      }
      else if(str[1][i]!='"') {
          new_args[index]=str[1][i];
          index++;
          i++;
      }
      else i++;    
  }
  new_args[index]='\0';
  if((strcmp(args[1], new_args)==0)&&(environmment_flag==0))
      printf("%s\n", new_args);
  else {
      strcpy(args[1], new_args);
      if(environmment_flag==1)
            environmment();
      else if(environmment_flag==0)
          printf("%s\n", new_args );
   }
}
void history_execute_with_constants() {
  int num, i, start_index;
  if(bang_flag==1) {
         for(i=0; i<no_of_lines; i++)
            printf("%s\n", history_data[i] ); 
  }
  else if(args[1]==NULL) {
        for(i=0; i<no_of_lines-1; i++)
            printf("%s\n", history_data[i] );
        printf(" %d %s\n", no_of_lines, his_var );
  }
  else {
      if(args[1]!=NULL)
          num = atoi(args[1]);
      if(num>no_of_lines) {
          for(i=0; i<no_of_lines-1; i++)
            printf("%s\n", history_data[i] );
          printf(" %d %s\n", no_of_lines, his_var );
      }
      else{
          start_index=no_of_lines-num;
          for(i=start_index; i<no_of_lines-1; i++)
              printf("%s\n", history_data[i] );
            printf(" %d %s\n", no_of_lines, his_var );
      }
  }
}
void issue_command() {
    int num,temp=1,temp_num,len,i,space=0;
    char ch[2]={"\n"};
    if(args[1]==NULL)
        printf("command incomplete issue expects an integer --> issue n \n");
    if(args[1]!=NULL) {
        num = atoi(args[1]);
        if(num>=no_of_lines)
            printf("error: %d exceeds number of commands given(%d)\n",num,no_of_lines);
        else {
            temp_num=num;
            while(temp_num>0) {
                temp_num=temp_num/10;
                temp++;
            }
            clear_variables();
            strcpy(input_buffer,history_data[num-1]);
            len = strlen(input_buffer);
            for(i=0;i<len-temp;i++)
                input_buffer[i]=input_buffer[temp+i];
            for(i=len-temp;i<len;i++)
                input_buffer[i+1]='\0';
            if(strcmp(input_buffer, ch)==0){
            //continue;
            }
            else {
                if(input_buffer[0]!='!') {
                    fileprocess();
                    //filewrite();
                }
                len = strlen(input_buffer);
                input_buffer[len-1]='\0';
                strcpy(his_var, input_buffer);
                if(strcmp(input_buffer, "exit") == 0)
                    flag = 1;
                if(input_buffer[0]=='!') {
                    fileprocess();
                    bang_flag=1;
                    bang_execute();
                }
                with_pipe_execute();
                waitpid(pid,&status,0);
                if(flag==1) {
                    printf("Exiting the Shell\n");
                    exit(0);
                }
            }
        }
    }
    clear_variables();
}
int is_dir(const char* path) {
    struct stat buf;
    stat(path, &buf);
    return S_ISDIR(buf.st_mode);
}
int lsh_launch(char **args) {
    pid_t pid;
    int status;
    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0) {
        // Error forking
        perror("lsh");
    }
    else {
        // Parent process
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}
char * get_files_folders(void) {
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    char * files=malloc(sizeof(char*)*1024);
    int index=0;
    if (d) {
        printf("\n");
        while ((dir = readdir(d)) != NULL) {
            for(int i=0;i<20;i++) {
                if((int)dir->d_name[i]==0)break;
                files[index++]=dir->d_name[i];
            }
            printf("\n");
            files[index++]=' ';
        }
        closedir(d);
    }
    files[index]='\n';
    return files;
}

int rmexcept(char **args) {
    char *files,*to_delete_files;
    if(args[1]==NULL)
        printf("%s",args[0]);
    files=get_files_folders();
    int i=0;
    while(files[i]!='\n')
        printf("%c",files[i++]);
    i=0;
    while(args[i]!=NULL)
        printf("%s ",args[i++]);
    printf("\n\n");
    i=0;
    char * file_name=malloc(sizeof(char)*100);
    char * cfile_name=malloc(sizeof(char)*100);
    char  ** launch_passing_args=malloc(sizeof(char)*3);
    int j=0;
    while(1) {
        if(files[i]=='\n')break;
        if(files[i]==' ') {
            int k=0;
            file_name[j]='\0';
            for(int t=0;t<=j;t++) {
                if(file_name[t]!=' ')cfile_name[k++]=file_name[t];
                file_name=cfile_name;
            }
            j=0;
            k=1;
            if(file_name[0]!='.') {
                int flag=0;
                while(args[k]!=NULL)
                    if(strcmp(file_name,args[k++])==0)flag=1;
                if(!flag) {
                    if(is_dir(file_name))launch_passing_args[0]="rmdir";
                    else launch_passing_args[0]="rm";
                    launch_passing_args[1]=file_name;
                    lsh_launch(launch_passing_args);
                }
            }
            char * file_name=malloc(sizeof(char)*100);
        }
        file_name[j++]=files[i++];
    }
    return 1;
}
static char* skipwhite(char* s) {
  while (isspace(*s)) ++s;
  return s;
}
void tokenise_commands(char *com_exec) {
    int m=1;
    args[0]=strtok(com_exec," ");
    while((args[m]=strtok(NULL," "))!=NULL)
        m++;
}
void tokenise_redirect_input_output(char *cmd_exec) {
  char *io_token[100];
  char *new_cmd_exec1;  
  new_cmd_exec1=strdup(cmd_exec);
  int m=1;
  io_token[0]=strtok(new_cmd_exec1,"<");       
  while((io_token[m]=strtok(NULL,">"))!=NULL)
        m++;
  io_token[1]=skipwhite(io_token[1]);
  io_token[2]=skipwhite(io_token[2]);
  input_redirection_file=strdup(io_token[1]);
  output_redirection_file=strdup(io_token[2]);
  tokenise_commands(io_token[0]);
}
void tokenise_redirect_input(char *cmd_exec) {
  char *i_token[100];
  char *new_cmd_exec1;  
  new_cmd_exec1=strdup(cmd_exec);
  int m=1;
  i_token[0]=strtok(new_cmd_exec1,"<");       
  while((i_token[m]=strtok(NULL,"<"))!=NULL)
        m++;
  i_token[1]=skipwhite(i_token[1]);
  input_redirection_file=strdup(i_token[1]);
  tokenise_commands(i_token[0]);
}
void tokenise_redirect_output(char *cmd_exec)
{
  char *o_token[100];
  char *new_cmd_exec1;  
  new_cmd_exec1=strdup(cmd_exec);
  int m=1;
  o_token[0]=strtok(new_cmd_exec1,">");       
  while((o_token[m]=strtok(NULL,">"))!=NULL)
          m++;
  o_token[1]=skipwhite(o_token[1]);
  output_redirection_file=strdup(o_token[1]); 
  tokenise_commands(o_token[0]);   
  
}
char* skipcomma(char* str)
{
  int i=0, j=0;
  char temp[1000];
  while(str[i++]!='\0') {
      if(str[i-1]!='"')
            temp[j++]=str[i-1];
  }
  temp[j]='\0';
  str = strdup(temp);
  return str;
}
static int split(char *cmd_exec, int input, int first, int last) {
    char *new_cmd_exec1;  
    new_cmd_exec1=strdup(cmd_exec);
   //else
      {
        int m=1;
        args[0]=strtok(cmd_exec," ");       
        while((args[m]=strtok(NULL," "))!=NULL)
              m++;
        args[m]=NULL;
        if (args[0] != NULL) {
            if (strcmp(args[0], "exit") == 0) 
                exit(0);
            if (strcmp(args[0], "echo") != 0) {
                  cmd_exec = skipcomma(new_cmd_exec1);
                  int m=1;
                  args[0]=strtok(cmd_exec," ");
                  while((args[m]=strtok(NULL," "))!=NULL)
                        m++;
                  args[m]=NULL;
            }
            if(strcmp("cd",args[0])==0) {
                    change_directory();
                    return 1;
            }
            else if(strcmp("pwd",args[0])==0){
                    parent_directory();
                    return 1;
            }
        }
      }
    return command(input, first, last, new_cmd_exec1);
}

void with_pipe_execute() {
    int i, n=1, input, first;
    input=0;
    first= 1;
    cmd_exec[0]=strtok(input_buffer,"|");
    while ((cmd_exec[n]=strtok(NULL,"|"))!=NULL)
        n++;
    cmd_exec[n]=NULL;
    pipe_count=n-1;
    for(i=0; i<n-1; i++) {
      input = split(cmd_exec[i], input, first, 0);
      first=0;
    }
    input=split(cmd_exec[i], input, first, 1);
    input=0;
    return;
}

void kill_child(int sig) {
    kill(pid,SIGKILL);
}

static int command(int input, int first, int last, char *cmd_exec) {
  int mypipefd[2], ret, input_fd, output_fd,m;
  if(args[1]!=NULL)
    m = atoi(args[1]);
  ret = pipe(mypipefd);
  if(ret == -1) {
        perror("pipe");
        return 1;
  }
  pid = fork();
  if (pid == 0) {
    if (first==1 && last==0 && input==0)
      dup2( mypipefd[1], 1 );
    else if (first==0 && last==0 && input!=0) {
      dup2(input, 0);
      dup2(mypipefd[1], 1);
    }
    else
      dup2(input, 0);
    if (strchr(cmd_exec, '<') && strchr(cmd_exec, '>')) {
              input_redirection=1;
              output_redirection=1;
              tokenise_redirect_input_output(cmd_exec);
     }
    else if (strchr(cmd_exec, '<')) {
          input_redirection=1;
          tokenise_redirect_input(cmd_exec);
    }
    else if (strchr(cmd_exec, '>')) {
          output_redirection=1;
          tokenise_redirect_output(cmd_exec);
    }
    if(output_redirection == 1) {
        output_fd= creat(output_redirection_file, 0644);
        if (output_fd < 0) {
              fprintf(stderr, "Failed to open %s for writing\n", output_redirection_file);
              return(EXIT_FAILURE);
        }
        dup2(output_fd, 1);
        close(output_fd);
        output_redirection=0;
    }
    if(input_redirection  == 1) {
        input_fd=open(input_redirection_file,O_RDONLY, 0);
        if (input_fd < 0) {
            fprintf(stderr, "Failed to open %s for reading\n", input_redirection_file);
            return(EXIT_FAILURE);
         }
         dup2(input_fd, 0);
         close(input_fd);
         input_redirection=0;
     }
     if (strcmp(args[0], "export") == 0) {
                  set_environment_variables();
                  return 1;
     }
     signal(SIGALRM,(void (*)(int))kill_child);
     alarm(m);
     if (strcmp(args[0], "echo") == 0) {
           echo_calling(cmd_exec);
     }
     else if (strcmp(args[0], "history") == 0) {
              history_execute_with_constants();
     }
     else if (strcmp(args[0],"issue")==0) {
            issue_command();
     }
     else if (strcmp(args[0],"rmexcept")==0) {
            rmexcept(args);
     }
     else if(execvp(args[0], args)<0){
         printf("%s: command not found\n", args[0]);
     }
     exit(0);
   }
  else {
     waitpid(pid, 0, 0);  
  }
  if (last == 1)
    close(mypipefd[0]);
  if (input != 0) 
    close(input);
  close(mypipefd[1]);
  return mypipefd[0];
}
void prompt() {
   char shell[1000];
   if (getcwd(cwd, sizeof(cwd)) != NULL){
          strcpy(shell, "Shell:");
          strcat(shell, cwd);
          strcat(shell, "$ ");
          printf("%s", shell);
   }
   else
       perror("getcwd() error");
}
int main() {
    int status;
    char ch[2]={"\n"};
    getcwd(current_directory, sizeof(current_directory));
    signal(SIGINT, sigintHandler);
    while (1) {
        clear_variables();
        prompt();
        fgets(input_buffer, 1024, stdin);
        if(strcmp(input_buffer, ch)==0){
              continue;
        }
        if(input_buffer[0]!='!'){
                fileprocess();
                filewrite(); 
        }
        len = strlen(input_buffer);
        input_buffer[len-1]='\0';
        strcpy(his_var, input_buffer);
        if(strcmp(input_buffer, "exit") == 0) {
              flag = 1;
              break;
         }
         if(input_buffer[0]=='!') {
                fileprocess();
                bang_flag=1;
                bang_execute();
          }
          with_pipe_execute();
          waitpid(pid,&status,0);
    }
    if(flag==1) {
        printf("Exiting the Shell\n");
        exit(0);
        return 0;
    }
    return 0;
}
