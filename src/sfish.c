#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <readline/readline.h>

#include "sfish.h"
#include "debug.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>

char cwd [1024];
char * cmdArr[999 * sizeof(char)];
int tokLength = 0;
char *afterP = " :: ahhchoi >> ";
int inputBool = 0;
int outputBool = 0;

int parseCmd(char * input){
    int index = 1;
    // getting the first token
    char *command = calloc(1024, sizeof(char));

    strcpy(command, input);
    char* tempInput = strtok(input, " ");
    cmdArr[0] = tempInput;
    while((tempInput= strtok(NULL, " ")) != NULL){
        cmdArr[index] = tempInput;
        index++;
    }
    // sing token cmd
    if(index == 1){
        if(strcmp(input, "help") == 0){
            printf("list of builtins \n help\n exit\n cd [- . ..]\n");
        }
        else if(strcmp(input, "exit") == 0){
            exit(3);
        }
        else if(strcmp(input, "cd") == 0){
            setenv("OLDPWD", getCwd(), 1);
            if(chdir(getenv("HOME")) == -1){
                perror(BUILTIN_ERROR);
                exit(EXIT_FAILURE);
            }
        }
        // else if(strcmp(input, "pwd") == 0){
        //     printf("%s\n", getCwd());
        // }
        else{
            if(hasSlash(command) == 1){ // if there is a slash
               char *argList[] = {input, NULL};
                executeSlash(input, argList);
            }
            else{
                char *argList[] = {input, NULL};
                execute(input, argList);
            }
        }
    }
    // 2 or more token cmd
    else if(strcmp(cmdArr[0], "cd") == 0){
            if(strcmp(cmdArr[1], "-") == 0){
                char * tempC = getCwd();
                chdir(getenv("OLDPWD"));
                setenv("OLDPWD", tempC, 1);
            }
            else if(strcmp(cmdArr[1], ".") == 0){ // do nothing
            }
            else if(strcmp(cmdArr[1], "..") == 0){
                //char * temp = getParent();
                setenv("OLDPWD", getCwd(), 1);
                char * newP = getParent();
                if(chdir(newP)== -1){
                    perror(BUILTIN_ERROR);
                    exit(EXIT_FAILURE);
                }
            }
            else{
                char* pp = getCwd();
                setenv("OLDPWD", pp, 1);
                strcat(pp, "/");
                strcat(pp, cmdArr[1]);
                if(chdir(pp)== -1){
                    perror(BUILTIN_ERROR);
                    exit(EXIT_FAILURE);
                }
            }
        }
    else{
        int pipeCount = 0;
        if((pipeCount = hasPipe(command)) > 0){
            executePipe(command, pipeCount);
        }
        else if(hasSymbol(command) == 1 || strcmp(cmdArr[0], "grep") == 0 || strcmp(cmdArr[0], "echo") || strcmp(cmdArr[0], "ls")){ // if there is a symbol
            executeRedirection(cmdArr, index, command);
        }
        else if(hasSlash(command) == 1){ // if there is a slash
            executeSlash(cmdArr[0], cmdArr);
        }
        else{
            execute(cmdArr[0], cmdArr);
        }

    }
    free(command);
    return 0;
}
int execute(char * cmd, char* argList []){
    pid_t pid;
    int status = 0;
    pid = fork();
    if(pid == 0){
        char* path;
        path = getenv ("PATH");
        char *pp = calloc(1024, sizeof(char));
        strcpy(pp, path);
        char* tok = strtok(pp, ":");
        int exFlag = 0;
        while(exFlag == 0 && tok != NULL){
            char * buff = calloc(1024, sizeof(char));
            strcpy(buff, tok);
            strcat(buff, "/");
            strcat(buff, cmd);
            struct stat *st = malloc(sizeof(struct stat));
            if(stat(buff, st) == 0){
                exFlag = 1;
                execv(buff, argList);
                break;
            }
            else{
                free(st);
                free(buff);
                tok = strtok(NULL, ":");
            }
        }
                free(pp);
        if(exFlag == 0){
            perror(EXEC_ERROR);
            return 1;
        }
    }
    else if(pid <0){
        perror(EXEC_ERROR);
        return 0;
    }
    else{
        waitpid(pid, &status, 0);
    }

    return 0;
}
int executeSlash(char * cmd, char* argList []){
    pid_t pid;
    int status = 0;
    pid = fork();
    if(pid == 0){
        struct stat *stt = malloc(sizeof(struct stat));
        if(stat(cmd, stt) == -1)
            perror(EXEC_ERROR);
        else{
            free(stt);
            execv(cmd, argList);
        }
    }
    else if(pid <0){
        perror(EXEC_ERROR);
    }
    else{
        waitpid(pid, &status, 0);
    }
    return 0;
}
int executeRedirection(char* argList[], int index, char* command){
    int infd = 0;
    int outfd = 1;
            char* infile = argList[findFileNameIndex(argList, "<", index)];
            char* outfile = argList[findFileNameIndex(argList, ">", index)];
        if(infile != NULL && outfile == NULL){
            infd = open(infile, O_RDONLY);
            if(infd < 0){
                perror(EXEC_ERROR);
            }
        }
        else if(outfile != NULL && infile == NULL){
            outfd = open(outfile, O_WRONLY | O_TRUNC | O_CREAT, S_IRGRP | S_IWUSR | S_IRUSR | S_IWGRP);
            if(outfd < 0){
                perror(EXEC_ERROR);
            }
        }
        else if(outfile != NULL && infile != NULL){
            infd = open(infile, O_RDONLY);
            outfd = open(outfile, O_WRONLY | O_TRUNC | O_CREAT, S_IRGRP | S_IWUSR | S_IRUSR | S_IWGRP);
            if(infd < 0 || outfd < 0){
                perror(EXEC_ERROR);

            }
        }
    int status = 0;
    pid_t pid;
    pid = fork();
    if(pid == 0){
        // making new argument array without a symbol
        char * newArg[300];
        for(int i = 0; i < 300; i++){
            newArg[i] = NULL;
        }

        int count = 0;
        for(int i = 0; i< index; i++){
            if(strcmp(argList[i], "<") != 0 && strcmp(argList[i], ">") != 0 && strcmp(argList[i], "|") != 0){
                newArg[count] = argList[i];
                count++;
            }
        }
        int dqCount = 0;
            if(inputBool == 1 && outputBool == 1){
                newArg[count-1] = NULL;
                newArg[count -2] = NULL;
            }
            else if(inputBool ==1 || outputBool == 1){
                newArg[count-1] = NULL;
            }
            else{
                if((dqCount = hasDQ(command)) > 0){
                    deleteDQ(newArg, count, dqCount);
                }
            }

        if(infile != NULL && outfile == NULL){
            dup2(infd, STDIN_FILENO);
            close(infd);
        }
        else if(outfile != NULL && infile == NULL){
            dup2(outfd, STDOUT_FILENO);
            close(outfd);
        }
        else if(outfile != NULL && infile != NULL){
            dup2(infd, STDIN_FILENO);
            dup2(outfd, STDOUT_FILENO);
            close(infd);
            close(outfd);
        }
            execvp(argList[0], newArg);
    }
    else if(pid < 0){
        perror(EXEC_ERROR);
    }
    else{
        waitpid(pid, &status, 0);
    }
    return 0;
}
int executePipe(char* cmd, int pipeCount){
    char * command = calloc(1024, sizeof(char));
    strcpy(command, cmd);
    char * ccc[300];
    char* tok = strtok(command, "|");
    int index = 0;
    ccc[0]= tok;
    while((tok= strtok(NULL, "|")) != NULL){
        index++;
        ccc[index] = tok;
    }
    // free(command);
//du | sort -nr | head
    int status;
    int i = 0;
    pid_t pid;

    int numPipe = pipeCount + 1;
    int pipefds[2*numPipe];
    for(i = 0; i < numPipe; i++){
        if(pipe(pipefds +i*2) < 0) {
            perror("pipe erorr");
            exit(EXIT_FAILURE);
        }
    }
    int j = 0;
    int x = 0;
    while(ccc[x] != 0) {
        pid = fork();

            // printf("%i &&&&& %i \n", pid, x);
        if(pid == 0) {
            if(ccc[x + 1]){
                if(dup2(pipefds[j+ 1], STDOUT_FILENO) < 0){
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }
            if(j != 0){
                if(dup2(pipefds[j-2], STDIN_FILENO) < 0){
                    perror("dup22");///j-2 0 j+1 1
                    exit(EXIT_FAILURE);
                }
            }
            //du | sort -nr | head
            for(i = 0; i < 2*numPipe; i++){
                    close(pipefds[i]);
            }
                char *cmdd = calloc(1024, sizeof(char));
                char* argg [300];
                strcpy(cmdd, ccc[x]);
                char* tempIn = strtok(cmdd, " ");
                argg[0] = tempIn;
                int t = 0;
                while((tempIn= strtok(NULL, " ")) != NULL){
                    t++;
                    argg[t] = tempIn;
                }
                argg[t+1] = NULL;
                free(cmdd);
                // printf("%s\n", argg[0]);
                if(execvp(argg[0], argg) < 0){
                        perror(EXEC_ERROR);
                        exit(EXIT_FAILURE);
                }
        }
        else if(pid < 0){
            perror(EXEC_ERROR);
            exit(EXIT_FAILURE);
        }
        x++;
        j+=2;
    }
    /**Parent closes the pipes and wait for children*/
    for(i = 0; i < 2 * numPipe; i++){
        close(pipefds[i]);
    }
    for(i = 0; i < numPipe + 1; i++)
        wait(&status);
    return 0;
}
// returning number of pipe
int hasPipe(char * cmd){
    int pipeCount = 0;
    char* temp = (char*)calloc(1024, sizeof(char));
    strcpy(temp, cmd);
    char* ptr = temp;
    while(*ptr){
        if(*ptr == '|'){
            pipeCount++;
        }
        ptr++;
    }
    free(temp);
    return pipeCount;
}
int hasSymbol(char* cmd){
    int symbolBool = 0;
    char* temp = (char*)calloc(1024, sizeof(char));
    strcpy(temp, cmd);
    char* ptr = temp;
    while(*ptr){
        if(*ptr == '<'){
            inputBool = 1;
            symbolBool = 1;
        }
        else if(*ptr == '>'){
            outputBool = 1;
            symbolBool = 1;
        }
        ptr++;
    }
    free(temp);
    return symbolBool;
}

int hasSlash(char* cmd){
    char* temp = (char*)calloc(1024, sizeof(char));
    strcpy(temp, cmd);
    char* ptr = temp;
    while(*ptr){
        if(*ptr == '/'){
            free(temp);
            return 1;
        }
        ptr++;
    }
    free(temp);
    return 0;
}
int hasDQ(char *cmd){
    int dqCount = 0;
    char* temp = (char*)calloc(1024, sizeof(char));
    strcpy(temp, cmd);
    char* ptr = temp;
    while(*ptr){
        if(*ptr == '"'){
            dqCount++;
        }
        ptr++;
    }
    free(temp);
    return dqCount;
}
char* getCwd(){
    char * cpwd = NULL;
    if((cpwd = getcwd(cwd, sizeof(cwd))) == NULL){
        fflush(stderr);
        return NULL;
    }
    return cpwd;
}
char* getPath(){
    char * P = getCwd();
    char *path = (char*) calloc(1024, sizeof(char));
    strcpy(path, P);

    // if it starts with are home/student
    char *temp = strtok(path, "/");
    char *tempArr[100];
    int index = 0;
    tempArr[0] = temp;
    while((temp= strtok(NULL, "/")) != NULL){
        index++;
        tempArr[index] = temp;
    }
    // rebuild the address
    if(strcmp(tempArr[0], "home") == 0){
            strcpy(path, "~");
            int i = 3;
            while(i<= index){
                strcat(path, "/");
                strcat(path, tempArr[i]);
                i++;
            }
    }
    strcat(path, afterP);
    strcpy(P, path);
    free(path);
    return P;
}
char* getParent(){
    char * P = getCwd();
    char *path = (char*) calloc(1024, sizeof(char));
    char *temp = strtok(P, "/");
    char * tempArr[50];
    int index = 0;

    tempArr[0] = temp;
    while((temp= strtok(NULL, "/")) != NULL){
        index++;
        tempArr[index] = temp;
    }
    for(int i = 0; i<index; i++){
        strcat(path, "/");
        strcat(path, tempArr[i]);
    }
    strcpy(P, path);
    free(path);
    return P;
}
int findFileNameIndex(char* argList[], char* symbol, int index){
    for(int i = 0; i< index; i++){
        if(strcmp(argList[i], symbol) == 0){
            return i + 1;
        }
    }
    return -1;
}
void deleteDQ(char * argList [], int count, int dqCount){
    int saveCount = 0;
    // check whether it has a pair
    for(int i = 0; i < count; i++){
        char * temp = calloc(1024, sizeof(char));
        strcpy(temp, argList[i]);
        if(*temp == '"'){
            saveCount = i;
        }
        free(temp);
    }

    strtok(argList[saveCount], "\"");
    strcpy(argList[saveCount], argList[saveCount]+(dqCount/2));
}
// void listDir(char * dirName)
// {
//     struct dirent *de;  // Pointer for directory entry

//     DIR *dr = opendir(dirName);

//     if (dr == NULL)  // opendir returns NULL if couldn't open directory
//     {
//         printf("Could not open current directory" );
//     }
//     while ((de = readdir(dr)) != NULL)
//             printf("%s  ", de->d_name);

//         printf("\n");
//     closedir(dr);
// }