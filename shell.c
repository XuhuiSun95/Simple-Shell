#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#define MAX_COMMAND_COUNT 3
#define MAX_TOKEN_LENGTH 32
#define MAX_TOKEN_COUNT 100
#define MAX_LINE_LENGTH 512

void cnt(int sig)
{
   static int count = 0;
   count++;
   if(count==2)
      exit(0);
}

void redirectin(char* in)
{
   int infd;
   
   infd = open(in, O_RDONLY);
   dup2(infd, 0);
   close(infd);
}

void redirectout(char* out)
{
   int outfd;

   outfd = open(out, O_CREAT|O_TRUNC|O_WRONLY, 0644);
   dup2(outfd, 1);
   close(outfd);
}

void runcommands(char* cmds[MAX_COMMAND_COUNT],int cmd_count, char* args[MAX_COMMAND_COUNT][MAX_TOKEN_COUNT], char* in, char* out)
{
   pid_t pid = fork();
   if(pid<0)
   {
      fprintf(stderr, "Fork Failed");
      exit(-1);
   }
   else if(pid==0)
   {
      if(cmd_count==1)
      {
         if(in!=NULL)
            redirectin(in);
         if(out!=NULL)
            redirectout(out);
         execvp(cmds[0], args[0]);
         perror(cmds[0]);
      }
      if(cmd_count==2)
      {
         int pipefd[2];

         pipe(pipefd);
         pid_t pid1 = fork();
         if(pid1<0)
         {
            fprintf(stderr, "Fork Failed");
            exit(-1);
         }
         else if(pid1==0)
         {
            if(out!=NULL)
               redirectout(out);
            dup2(pipefd[0], 0);
            close(pipefd[0]);
            close(pipefd[1]);
            execvp(cmds[1], args[1]);
            perror(cmds[1]);
         }
         else 
         {
            if(in!=NULL)
               redirectin(in);
            dup2(pipefd[1], 1);
            close(pipefd[0]);
            close(pipefd[1]);
            execvp(cmds[0],args[0]);
            perror(cmds[0]);
            waitpid(pid1, NULL, 0);
         }
      }
      if(cmd_count==3)
      { 
         int pipefd[4];

         pipe(pipefd);
         pipe(pipefd + 2);
         pid_t pid2 = fork();
         if(pid2<0)
         {
            fprintf(stderr, "Fork Failed");
            exit(-1);
         }
         else if(pid2==0)
         {
            if(in!=NULL)
               redirectin(in);
            dup2(pipefd[1], 1);
            close(pipefd[0]);
            close(pipefd[1]);
            close(pipefd[2]);
            close(pipefd[3]);
            execvp(cmds[0], args[0]);
            perror(cmds[0]);
         }
         else 
         {
            pid_t pid3 = fork();
            if(pid3<0)
            {
               fprintf(stderr, "Fork Failed");
               exit(-1);
            }
            else if(pid3==0)
            {
               dup2(pipefd[0], 0);
               dup2(pipefd[3], 1);
               close(pipefd[0]);
               close(pipefd[1]);
               close(pipefd[2]);
               close(pipefd[3]);
               execvp(cmds[1], args[1]);
               perror(cmds[1]);
            }
            else
            {
               if(out!=NULL)
                  redirectout(out);
               dup2(pipefd[2],0);
               close(pipefd[0]);
               close(pipefd[1]);
               close(pipefd[2]);
               close(pipefd[3]);
               execvp(cmds[2], args[2]);
               perror(cmds[2]);
               waitpid(pid3, NULL, 0);
            }
            waitpid(pid2, NULL, 0);
         }
      }
   }
   else 
   {
      waitpid(pid, NULL, 0);
   }
}

int main()
{
   signal(SIGTSTP, cnt);

   char line[MAX_LINE_LENGTH];

   //printf("basic shell: $ ");
   while(fgets(line, MAX_LINE_LENGTH, stdin))
   {
      line[strlen(line)-1] = '\0';
      char* pipe[MAX_COMMAND_COUNT];
      char* command[MAX_COMMAND_COUNT];
      char* arguments[MAX_COMMAND_COUNT][MAX_TOKEN_COUNT];
      int command_count = 0;
      int argument_count[MAX_COMMAND_COUNT] = {0, 0, 0};
      char* in = NULL;
      char* out = NULL;

      char* token_c = strtok(line, "|");
      while(token_c)
      {
         pipe[command_count] = token_c;
         command_count++;
         if(command_count==3)
            break;
         token_c = strtok(NULL, "|");
      }

      for(int i=0; i<command_count; i++)
      {
         char* cmd = NULL;
         char* args[MAX_TOKEN_COUNT];
         int arg_count = 0;

         char * token_a = strtok(pipe[i], " ");
         while(token_a)
         {
            if(!cmd)
               cmd = token_a;
            args[arg_count] = token_a;
            arg_count++;
            token_a = strtok(NULL, " ");
         }
         for(int j=arg_count-1; j>0; j--)
         {
            if(strcmp(args[j], ">") ==0)
            {
               out = args[j+1];
               arg_count -=2;
            }
            if(strcmp(args[j], "<") ==0)
            {
               in = args[j+1];
               arg_count -=2;
            }
         }

         command[i] = cmd;
         argument_count[i] = arg_count;
         for(int k=0; k<arg_count; k++)
         {
            arguments[i][k] = args[k];
         }
         arguments[i][arg_count] = NULL;
      }

      if(strcmp(command[0], "exit")==0)
         exit(0);
      runcommands(command, command_count, arguments, in, out);

      //printf("basic shell: $ ");
   }
   return 0;
}
