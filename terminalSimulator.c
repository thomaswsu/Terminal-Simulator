#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h> // isalpha()
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <dirent.h> // opendir()

 int child(char* command, char** arguments)
{
    execv(command, arguments);
    return(EXIT_SUCCESS);
}

int backgroundChild(char* command, char** arguments)
{
    #ifdef DEBUG_MODE
        for(int i = 0; i < 16; i++)
        {
            printf("%s\n", arguments[i]);
        }
    #endif

    printf("[running background process \"%s\"]\n", arguments[0]);
    execv(command, arguments);    
    return(EXIT_SUCCESS);
}

int child_pipe_read(char* command, char** arguments, int readPipe, int writePipe)
{
    
    #ifdef DEBUG_MODE
        printf("Now at pipe read\n");
        printf("pipe read command: %s\n", command);
        for(int i = 0; i < 16; i++)
        {
            printf("arg%d: %s\n", i, arguments[i]);
        }
    #endif
    

    close(writePipe);
    dup2(readPipe, STDIN_FILENO);
    execv(command, arguments);
    close(readPipe);
    return(EXIT_SUCCESS);
}

int child_pipe_write(char* command, char** arguments, int readPipe, int writePipe)
{  
    #ifdef DEBUG_MODE
        printf("Now at pipe write\n");
        printf("pipe write command: %s\n", command);
    #endif

    close(readPipe);
    dup2(writePipe, STDOUT_FILENO);
    execv(command, arguments);
    close(writePipe);
    return(EXIT_SUCCESS);
}

void setArgumentsToNull(char** argumentsArray, unsigned int lenArguments)
{
    for(unsigned int i = 0; i < lenArguments; i++)
    {
        *(argumentsArray + i) = NULL;
    }
    return;
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    DIR *dir = opendir(".");  /* open the current working directory */

    if (dir == NULL)
    {
        perror( "opendir() failed" );
        return EXIT_FAILURE;
    }

    char input[1024]; // This is to get the all the commands
    
    char path[1024];
    if(getenv("MYPATH") == NULL)
    {
        strcpy(path, "/bin:.");
    }
    else
    {
        strcpy(path, getenv("MYPATH"));

    }
    // don't forget to free path
    /* bool null_path = false; 
    if(PATH == NULL)
    null_path = true; 
    Path = calloc(7, sizofchar(char);
    strcpypath(PATH, "/bin:."*/

    char originalPath[1024];
    strcpy(originalPath, path);
    
    char directory[1024];
    pid_t pid;
    int ampersand = 0;
    int usePipe = 0;
    int rc = 0;
    //char buffer[2048];
    
    char* args1[64]; // To get arguments
    char* args2[64]; // If pipe is used 

    for(int i = 0; i < 64; i++)
    {
        args1[i] = NULL;
        args2[i] = NULL;
    }

    #ifdef DEBUG_MODE
        printf("The current path is %s\n", path);
    #endif

    if(rc == -1)
    {
        perror("Error: <pipe() failed>");
        return(EXIT_FAILURE);
    }
    loop: // We get new input if there is no input
        while(1)
        {
            int status;
            int childEnd;
            do 
            {
                childEnd = waitpid(-1, &status, WNOHANG); 
                if(childEnd > 0 && status == 0)
                {
                    printf("[process %d terminated with exit status %d]\n", childEnd, WEXITSTATUS(status));
                }
            } while(childEnd == ECHILD);

            getcwd(directory, 1024);
            printf("%s$ ", directory);
            fgets(input, 1024, stdin);
            if(strlen(input) <= 1)
            {
                setArgumentsToNull(args1, 16); // Avoid arguments spilling into other commands
                setArgumentsToNull(args2, 16);
                ampersand = 0;
                usePipe = 0;
                goto loop; // if empty
            } 
            *(input +  (strlen(input) - 1) ) = '\0'; // Get rid of stupid newline
            
            // If there is an ampersand at the end
            if((*(input + (strlen(input) - 1)) == '&') && \
            (*(input + (strlen(input) - 2)) == ' '))
            {
                (*(input + (strlen(input) - 2)) = '\0'); // Get rid of ampersand in input 
                ampersand = 1;
            }

            #ifdef DEBUG_MODE
                printf("The input is: %s\n", input);
                printf("The directory is: %s\n", path);
            #endif

            char* token = strtok(input, " ");
            unsigned int index1 = 0;
            unsigned int index2 = 0;
            while(token != NULL) // Get all arguments
            {           
                if(strcmp(token, "|") == 0)
                {
                    usePipe = 1;
                    token = strtok(NULL, " "); // skipover one, leave out |
                    #ifdef DEBUG_MODE
                        printf("Pipe used!\n");
                    #endif
                }
        
                if(usePipe)
                {
                    *(args2 + index2) = token;
                    token = strtok(NULL, " ");
                    index2++;
                }
                else
                {
                    *(args1 + index1) = token;    
                    token = strtok(NULL, " "); 
                    index1++;
                }
            }

            char truePath2[1024]; // Find path
            strcpy(path, originalPath);
            char* token2 = strtok(path, ":");
            while(token2 != NULL)
            {
                strcat(truePath2, token2);
                strcat(truePath2, "/");
                strcat(truePath2, args1[0]);
                
                #ifdef DEBUG_MODE
                    printf("TruePath2: %s\n", truePath2);
                #endif

                struct stat buffer;
                if(lstat(truePath2, &buffer) == 0)
                {
                    #ifdef DEBUG_MODE
                        printf("Found path2: %s\n", truePath2);
                    #endif
                    break;
                }
                token2 = strtok(NULL, ":");
                truePath2[0] = '\0'; // If not found indicate that executable not found 
            }
            char truePath3[1024]; // Find path
            strcpy(path, originalPath);
            char* token3 = strtok(path, ":"); // This is for if the pipe operator is used
            while(token3 !=  NULL && usePipe)
            {
                strcat(truePath3, token3);
                strcat(truePath3, "/");
                strcat(truePath3, args2[0]);
                
                #ifdef DEBUG_MODE
                    printf("TruePath3: %s\n", truePath3);
                #endif

                struct stat buffer;
                if(lstat(truePath3, &buffer) == 0)
                {
                    #ifdef DEBUG_MODE
                        printf("Found path3: %s\n", truePath3);
                    #endif
                    break;
                }
                token3 = strtok(NULL, ":");
                truePath3[0] = '\0';
            }
            

            #ifdef DEBUG_MODE
                printf("The executable name is %s\n", args1[0]);

                for(int i = 1; i < index1; i++)
                // -2 becuase we over count by one, and executable is not an argument
                {
                    printf("Argument %d is: %s\n", i, *(args1 + i));
                }
            #endif
            

            if(strcmp(args1[0], "exit") == 0)
            {
                printf("bye\n");
                closedir(dir);
                return(EXIT_SUCCESS);
            }

            if(strcmp(args1[0], "cd") == 0)
            {
                if(args1[1] == NULL)
                {
                    if(chdir("$HOME") == -1)
                    {
                        perror("chdir() failed");
                    }
                }
                else
                {
                    if(chdir(args1[1]) == -1)
                    {
                        perror("chdir() failed");
                    }
                }
                setArgumentsToNull(args1, 16); // Avoid arguments spilling into other commands
                setArgumentsToNull(args2, 16);
                ampersand = 0;
                usePipe = 0;
                goto loop;
            }


            if(truePath2[0] == '\0')// && (!usePipe || (truePath3[0] == '\0')))
            {
                #ifdef DEBUG_MODE
                    printf("Error: <%s is not a valid executable.>\n", args1[0]);
                #endif
                fprintf(stderr, "ERROR: command \"%s\" not found\n", args1[0]);

                setArgumentsToNull(args1, 16); // Avoid arguments spilling into other commands
                setArgumentsToNull(args2, 16);
                ampersand = 0;
                usePipe = 0;
                goto loop;
            }
            else if (usePipe && ampersand)
            {
                int p[2];
                rc = pipe(p);
                if(rc == -1)
                {
                    perror("Error: <pipe() failed>");
                    return(EXIT_FAILURE);
                }
                
                pid = fork();
                if(pid == -1)
                {
                    perror("Error: <fork failed>");
                    return(EXIT_FAILURE);
                }
                else if(pid == 0)
                {
                    #ifdef DEBUG_MODE
                        printf("executePath: %s\n",truePath2);
                    #endif    
                    child_pipe_write(truePath2, args1, p[0], p[1]);
                    close(p[1]); // Close write end
                    exit(rc);  
                }
                else
                {
                     waitpid(0, NULL, 0); // Wait for child process. 
                     // We always have to do this or else we might end up reading from an empty pipe
                     close(p[1]); // Close write end
                     pid = fork();

                     if(pid == - 1)
                     {
                         perror("Error: <fork failed>");
                         return(EXIT_FAILURE);
                     }
                     else if(pid == 0)
                     {
                        child_pipe_read(truePath3, args2, p[0], p[1]);
                        close(p[0]); // Close read end
                     }
                }
            }
            else if(usePipe)
            {
                #ifdef DEBUG_MODE
                    printf("Using pipe without background\n");
                    printf("Begining truePath2: %s\n", truePath2);
                    printf("Begining truePath3: %s\n", truePath3);
                #endif
                int p[2];
                rc = pipe(p);
                if(rc == -1)
                {
                    perror("Error: <pipe() failed>");
                    return(EXIT_FAILURE);
                }
                
                pid = fork();
                if(pid == -1)
                {
                    perror("Error: <fork failed>");
                    return(EXIT_FAILURE);
                }
                else if(pid == 0)
                {
                    #ifdef DEBUG_MODE
                        printf("executePath: %s\n",truePath2);
                    #endif    
                    child_pipe_write(truePath2, args1, p[0], p[1]);
                    close(p[1]); // Close write end
                    exit(rc);  
                }
                else
                {
                     waitpid(0, NULL, 0); // Wait for child process. 
                     // We always have to do this or else we might end up reading from an empty pipe
                     close(p[1]); // Close write end
                     pid = fork();

                     if(pid == - 1)
                     {
                         perror("Error: <fork failed>");
                         return(EXIT_FAILURE);
                     }
                     else if(pid == 0)
                     {
                        child_pipe_read(truePath3, args2, p[0], p[1]);
                        close(p[0]); // Close read end
                     }
                     else
                     {  
                         if(!ampersand) waitpid(0, NULL, 0);
                     }
                }
            }
            else if(ampersand)
            {
                #ifdef DEBUG_MODE
                    printf("executable path is %s\n", truePath2);
                    printf("%s is a valid executable.\n", args1[0]);
                #endif
            
                pid = fork(); // Create a new child process to execute the desired program
                if(pid == -1)
                {
                    perror("Error: <fork failed>");
                    return(EXIT_FAILURE);
                }
                else if(pid == 0)
                {
                    #ifdef DEBUG_MODE
                        printf("executePath: %s\n",truePath2);
                    #endif
                    backgroundChild(truePath2, args1);
                }
                else
                {

                }
            }
            else // the executable is valid
            {
                #ifdef DEBUG_MODE
                    printf("executable path is %s\n", truePath2);
                    printf("%s is a valid executable.\n", args1[0]);
                #endif
            
                pid = fork(); // Create a new child process to execute the desired program
                if(pid == -1)
                {
                    perror("Error: <fork failed>");
                    return(EXIT_FAILURE);
                }
                else if(pid == 0)
                {
                    #ifdef DEBUG_MODE
                        printf("executePath: %s\n",truePath2);
                    #endif
                    child(truePath2, args1);  
                }
                else
                {
                    waitpid(pid, NULL, 0);
                }
            }

           setArgumentsToNull(args1, 16); // Avoid arguments spilling into other commands
           setArgumentsToNull(args2, 16);
           
           ampersand = 0;
           usePipe = 0;
        }
    closedir(dir);
    return(EXIT_SUCCESS);
}

/* Sources:
https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/
https://www.ibm.com/support/knowledgecenter/en/ssw_ibm_i_72/apis/lstat.htm
https://stackoverflow.com/questions/51310985/remove-newline-from-fgets-input
https://stackoverflow.com/questions/28507950/calling-ls-with-execv
https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.1.0/com.ibm.zos.v2r1.bpxbd00/rtwaip.htm
https://stackoverflow.com/questions/10700982/redirecting-stdout-to-pipe-in-c
*/
