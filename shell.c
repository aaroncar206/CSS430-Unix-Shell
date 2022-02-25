/*
* shell.c 
* 
* Author: Aaron Carbonell and Vu Tieu
*
* This a C++ program that simulates a simple Unix shell.
* 
* Can execute the following commands:
* ls -l
* ls -al
* ls & whoami
* !! 
* ls > fileName.txt
* cat < fileName.txt
* ls | wc (semi functional)
*
*/
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>

#define MAX_LINE 80 /* The maximum length command */


bool needsWait(char cmdLine[1024]);
int redirectIndex(char *args[MAX_LINE/2 + 1], char symbol);
bool hasSymbol(char* args[MAX_LINE/2 + 1], char symbol);

int main(void)
{
  char commandLine[1024];
  char *args[MAX_LINE/2 + 1]; /* command line arguments */
  char *history[MAX_LINE/2 + 1];
  int should_run = 1; /* flag to determine when to exit program */
  
  while (should_run) {
    printf("osh> ");
    fflush(stdout);

    fgets(commandLine, 1024, stdin); //read in user command 
    char* cmd; 

    commandLine[strcspn(commandLine, "\n")] = 0;

    cmd = strtok(commandLine, " "); //separate user command into tokens by space
    int counter = 0; 

    if(strcmp(cmd,"!!")==0){        // if command is !! copy history to args
      int i = 0;
      while(history[i] != NULL){
        args[i] = (char*)malloc(80 * sizeof(char));
        strcpy(args[i], history[i]);
        i++;
      }
    }
    else{
      while(cmd != NULL) //store each token into args 
      {
        args[counter] = cmd; 
        
        history[counter] = (char*)malloc(80 * sizeof(char));
        strcpy(history[counter], cmd);                       

        cmd = strtok(NULL, " ");
        ++counter;
      }
    }

    


    // for (int i = 0; i <= counter; i++){
    //   printf("args[%d]: %s\n", i, args[i]);
    // }

    /*
    * After reading user input, the steps are:
    * (1) fork a child process using fork()
    * (2) the child process will invoke execvp()
    * (3) parent will invoke wait() unless command included &
    */

    pid_t pid; 
    pid = fork(); //fork the child process 
    printf("pid: %d", pid); 

    if(pid < 0) //invalid process
    {
      perror("fork error"); 
      exit(1); 
    }
    else if (pid == 0) //the child process
    {
      if(args[0] == NULL){
        printf("No commands in history.\n");
      }


      // redirect operators
      if(strcmp(args[1], ">") == 0) //check if reading from file or writing to
      {
        int fileOut = open(args[2], O_WRONLY | O_CREAT, 0777); //open textfile to output
        dup2(fileOut, STDOUT_FILENO); //replace stdout fd with text file 
        close(fileOut); //no longer need file
        args[1] = NULL; //clear the remaining args so only executes first command
        args[2] = NULL;
      }
      else if(strcmp(args[1], "<") == 0)
      {
        int fileIn = open(args[2], O_RDONLY | O_CREAT, 0777); //open textfile to read input 
        dup2(fileIn, STDIN_FILENO); //replace stdin fd with text file
        close(fileIn); //no longer need file
        args[1] = NULL; //clear the remaining args so only executes first command
        args[2] = NULL;
      }

      execvp(args[0], args); //child process invokes execvp
    }
    else //parent process
    {
      printf("run parent process\n");
      int status; 
      printf("status %d" ,status);

      while(counter > 0){       // clear args
        args[counter] = NULL;
        counter--;
      }

      if(needsWait(commandLine)) //check if needs to wait for next process
      {
        wait(&status); //parent invokes wait
      }
    }
  } 
  return 0;
}

/*
* check if a command needs to wait to execute next process
* (does not have an '&' in it)
*
*/
bool needsWait(char cmdLine[1024])
{

  int numChars = sizeof(cmdLine) / sizeof(cmdLine[0]);
  for(int i = 0; i < numChars; i++)
  {
    if(cmdLine[i] =='&')
    {
      return false; 
    }
    
    if(cmdLine[i] == ';')
    {
      return true; 
    }
  }

  return true; 
}

int redirectIndex(char *args[MAX_LINE/2 + 1], char symbol)
{
  int counter = 0; 

  while(args[counter] != NULL)
  {
    if((strcmp(args[counter], symbol) == 0))
    {
      return counter; 
    }
  }

  return -1; 
}

bool hasSymbol(char* args[MAX_LINE/2 + 1], char symbol)
{
  int counter = 0; 

  while(args[counter] != NULL)
  {
    if((strcmp(args[counter], symbol) == 0))
    {
      return true; 
    }
  }

  return false;
}

      /* int pipeIdx = pipeIndex(args, "|"); 
      if(pipeIndex > 0) //check for pipe command 
      {
        int fd[2]; 
        if(pipe(fd) == -1) //create pipe
        {
          return 1; 
        }

        char *leftProc[MAX_LINE/2 + 1]; 
        char *rightProc[MAX_LINE/2 + 1]; 

        //copy process to the left of pipe into other array 
        for(int i = 0; i < pipeIdx; i++)
        {
          leftProc[i] = (char*)malloc(80 * sizeof(char)); 
          strcpy(leftProc[i], args[i]);
        }

        //copy process to the right of pipe into other array 
        int j = 0;
        for(int i = pipeIdx; args[i] != NULL; i++)
        {
           
          rightProc[j] = (char*)malloc(80 * sizeof(char));
          strcpy(rightProc[j], args[i]);
          j++; 
        }


        dup2(fd[1], STDOUT_FILENO); //direct left process output to write end
        close(fd[0]); 
        close(fd[1]); 
        execvp(leftProc[0], leftProc); //execute left process 

        int pid2 = fork(); //child process creates another child process 
        if(pid2 < 0)
        {
          perror("fork error"); 
          exit(2); 
        }

        if(pid2 == 0)
        {
          dup2(fd[0], STDIN_FILENO); //direct right process output to read end 
          close(fd[0]);
          close(fd[1]); 
          execvp(rightProc[0], rightProc); //execute write process 
        }

        close(fd[0]); 
        close(fd[1]);

        waitpid(pid, NULL, 0); 
        waitpid(pid2, NULL, 0); 
      }
      else
      { */
/*
* shell.c 
* 
* Author: Aaron Carbonell and Vu Tieu
*/
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>

#define MAX_LINE 80 /* The maximum length command */


bool needsWait(char cmdLine[1024]);
int pipeIndex(char *args[MAX_LINE/2 + 1], char symbol);

int main(void)
{
  char commandLine[1024];
  char *args[MAX_LINE/2 + 1]; /* command line arguments */
  char *history[MAX_LINE/2 + 1];
  int should_run = 1; /* flag to determine when to exit program */
  
  
  while (should_run) {
    printf("osh> ");
    fflush(stdout);

    fgets(commandLine, 1024, stdin); //read in user command 
    char* cmd; 

    commandLine[strcspn(commandLine, "\n")] = 0;

    cmd = strtok(commandLine, " "); //separate user command into tokens by space
    int counter = 0; 

    if(strcmp(cmd,"!!")==0){        // if command is !! copy history to args
      int i = 0;
      while(history[i] != NULL){
        args[i] = (char*)malloc(80 * sizeof(char));
        strcpy(args[i], history[i]);
        i++;
      }
    }
    else{

      int i = 0; 
      while(history[i] != NULL)
      {
        history[i] = NULL;
        i++; 
      }

      while(cmd != NULL) //store each token into args 
      {
        args[counter] = cmd; 
        
        history[counter] = (char*)malloc(80 * sizeof(char));
        strcpy(history[counter], cmd);                       

        cmd = strtok(NULL, " ");
        ++counter;
      }
    }

    // for (int i = 0; i <= counter; i++){
    //   printf("args[%d]: %s\n", i, args[i]);
    // }

    /*
    * After reading user input, the steps are:
    * (1) fork a child process using fork()
    * (2) the child process will invoke execvp()
    * (3) parent will invoke wait() unless command included &
    */

    pid_t pid; 
    pid = fork(); //fork the child process 
    printf("pid: %d", pid); 

    //splice the array by command into the commands.
    
    if(pid < 0) //invalid process
    {
      perror("fork error"); 
      exit(1); 
    }
    else if (pid == 0) //the child process
    {
      if(args[0] == NULL){
        printf("No commands in history.\n");
      }

        int i = 0; 
        while(args[i] != NULL)
        {
            // redirect operators
          if(strcmp(args[i],">") == 0) //check if reading from file or writing to
          {
            int fileOut = open(args[i+1], O_WRONLY | O_CREAT, 0777); //open textfile to output
            dup2(fileOut, STDOUT_FILENO); //replace stdout fd with text file 
            close(fileOut); //no longer need file
            args[i] = NULL;
            args[i+1] = NULL;
            
          }
          else if(strcmp(args[i],"<") == 0)
          {
            int fileIn = open(args[i+1], O_RDONLY | O_CREAT, 0777); //open textfile to read input 
            dup2(fileIn, STDIN_FILENO); //replace stdin fd with text file
            close(fileIn); //no longer need file
            args[i] = NULL;
            args[i+1] = NULL;
          }

          ++i;
        }

        execvp(args[0], args); //child process invokes execvp
      //}
      
    }
    else //parent process
    {
      printf("run parent process\n");
      int status; 
      printf("status %d" ,status);

      while(counter > 0){       // clear args
        args[counter] = NULL;
        counter--;
      }

      if(needsWait(commandLine)) //check if needs to wait for next process
      {
        wait(&status); //parent invokes wait
      }
    }
  } 
  return 0;
}

/*
* check if a command needs to wait to execute next process
* (does not have an '&' in it)
*
*/
bool needsWait(char cmdLine[1024])
{

  int numChars = sizeof(cmdLine) / sizeof(cmdLine[0]);
  for(int i = 0; i < numChars; i++)
  {
    if(cmdLine[i] =='&')
    {
      return false; 
    }
    
    if(cmdLine[i] == ';')
    {
      return true; 
    }
  }

  return true; 
}

int pipeIndex(char *args[MAX_LINE/2 + 1], char symbol)
{
  int counter = 0; 

  while(args[counter] != NULL)
  {
    if((strcmp(args[counter], symbol) == 0))
    {
      return counter; 
    }
  }

  return -1; 
}
