#ifndef SFISH_H
#define SFISH_H

/* Format Strings */
#define EXEC_NOT_FOUND "sfish: %s: command not found\n"
#define JOBS_LIST_ITEM "[%d] %s\n"
#define STRFTIME_RPRMT "%a %b %e, %I:%M%p"
#define BUILTIN_ERROR "sfish builtin error"
#define SYNTAX_ERROR "sfish syntax error"
#define EXEC_ERROR "sfish exec error"


#endif

int parseCmd(char *);
char* getCwd();
char* getParent();
char* getPath();

int hasSlash(char* cmd);
int hasSymbol(char* cmd);
int hasPipe(char * cmd);
int hasDQ(char *cmd);

int execute(char * cmd, char* argList []);
int executeSlash(char * cmd, char* argList []);
int executeRedirection(char* argList[], int index, char* command);
int executePipe(char * command, int pipeCount);

int findFileNameIndex(char* argList[], char* symbol, int index);
char * getNewArray(char * argList[], int index);
void deleteDQ(char * argList [], int count, int dqCount);