#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFF_SIZE 1000
void child_task(char* arg[] , int argc) ;
int valid_command(char const* command)
{
    // printf("Command is '%s'\n",command );
    struct stat sb ;
    return(stat(command , &sb) == 0 && S_ISREG(sb.st_mode));
    /*
    FILE *file ;
    if(file = fopen(command , "r"))
    {
        fclose(file) ;
        return 1 ;
    }
    // if(command[0] == '/' && command[1] == 'b' && command[2] == 'i' && command[3] == 'n')
        // return 1 ;
    return 0 ;
    */
}
void executing_pipe(char* argv[] ,int pipe_ind , int argc ) {
    // for (size_t i = 0; i < argc; i++) {
    //     printf("argv[%ld] = %s\n", i , argv[i] );
    // }
    // printf("%d %d\n",pipe_ind , argc );
    int fd[2] ;
    int ret = pipe(fd) ;
    if(ret == -1)
    {
        write(2,"Pipe Error" , 10); return ;
    }
    int pid = fork() ;
    // printf("%d\n",pid );
    if(pid == 0 )   // child
    {
        close(fd[0]) ;
        close(1) ;
        dup(fd[1]) ;
        close(fd[1]) ;
        // argv[pipe_ind] = NULL ;
        // printf("executing_pipe : %s \n",argv[0] );
        // printf("her\n");
        // write(2,"child\n" , 6);
        child_task(argv , pipe_ind ) ;
        // write(2,"child\n" , 6);
        // printf("child\n" );
        // execv(argv[0],argv) ;
    }
    else if(pid > 0)
    {
        close(fd[1]) ;
        close(0) ;
        dup(fd[0]) ;
        close(fd[0]) ;

        if(!valid_command(argv[pipe_ind]))
        {
            write(2 , "Invalid Command\n" , 16) ;
            exit(EXIT_SUCCESS) ;
        }

        child_task(&(argv[pipe_ind]) , argc - pipe_ind) ;
        // write(2,"parent\n" , 7);
    }
    else
        write(2 ,"Fokking Error\n" ,14) ;
}
int update_arguments(char* command , char **argv) {
    int i = 0 , j = 0 , valid_traverse_flag = 0 ;
    while (command[j] != '\0') {
        if(valid_traverse_flag == 0 && command[j] != ' ')
        {
            if(command[j] == '>' || command[j] == '<' || (command[j+1] == '>' && (command[j] == '1' || command[j] == '2')))
                argv[i++] = '\0' ;
            argv[i++] = command + j ;
            valid_traverse_flag = 1 ;
        }
        else if(valid_traverse_flag == 1 && command[j] == ' ')
        {
            command[j] = '\0' ;
            valid_traverse_flag = 0 ;
        }
        j++ ;
    }
    argv[i++] = NULL ; // or '\0'
    return i ; // len of argv ;
}
void child_task(char* argv[] , int argc )
{
    int flag = -1 , pipe_ind = -1 ;
    char* filename ;
    for(int i = 0 ; i < argc ; i++)
    {   // FIXME : this may lead to invalid command execution ><
        // printf("argv[%d]  = '%s'\n", i , argv[i]);
        if(argv[i] == NULL)
        {
            if(i+1 == argc && flag == -1) flag = 0 ;
            continue ;
        }
        if(strcmp(argv[i] , ">") == 0)
        {
            filename = argv[i+1] ;
            flag = 1 ;
            // break ;
        }
        else if(strcmp(argv[i] , "<") == 0)
        {
            filename = argv[i+1] ;
            flag = 6 ;
            // break ;
        }
        else if(strcmp(argv[i] , ">>") == 0)
        {
            filename = argv[i+1] ;
            flag = 2 ;
            // break ;
        }
        else if(argv[i][0] != '\0' && argv[i][1] == '>')
        {
            if(argv[i][0] == '1')
            {
                close(1) ;
                creat((char *)&argv[i][2] , 0666) ;
            }
            else if(argv[i][0] == '2')
            {
                if(argv[i][2] != '&')
                {
                    close(2) ;
                    creat((char*)&argv[i][2] , 0666) ;
                }
                else
                {
                    close(2) ;
                    dup(1) ;
                }
            }
        }
        else if(strcmp(argv[i] , "|") == 0)
        {
            flag = 7 ;
            argv[i] = NULL ;
            pipe_ind = i+1 ;
        }
    }
    // printf("Case and len is %d , %d\n",flag , argc );
    switch (flag) {
        case -1:write(2 , "Invalid command at some_point" , 29) ;
        case 0: execv(argv[0] , argv) ;   break ;
        case 1: close(1) ;
                creat(filename , 0666) ;
                execv(argv[0] , argv) ;   break ;
        case 2: close(1) ;
                mode_t mode = S_IRUSR	| S_IWUSR	| S_IXUSR	| S_IRGRP	| S_IWGRP	| S_IXGRP	| S_IROTH	| S_IWOTH	| S_IXOTH ;
                open(filename , O_WRONLY | O_APPEND | O_CREAT , mode ) ;
                execv(argv[0] , argv) ;   break ;
        // case 3:printf("Not implemented yet\n"); break ; // TODO : implemente this
        // case 4:printf("Not implemented yet\n"); break ;
        // case 5:printf("Not implemented yet\n"); break ;
        case 6: close(0) ;
                open(filename , O_RDONLY) ;
                execv(argv[0] , argv) ;   break ;
        case 7: executing_pipe(argv, pipe_ind , argc);   break ;

    }
}
void execute(char *const arg[] , int argc)
{
    if(!valid_command(arg[0]))
    {
        write(2 , "Invalid Command\n" , 16) ;
        return ;
    }
    int pid = fork() ;
    // printf("%d\n",pid );
    if(pid == 0 )   // child
        child_task((char**)arg,argc) ;        // execv(arg[0] , arg) ;
    else if(pid > 0)
        wait(NULL) ;
    else
        write(2 ,"Fokking Error\n" ,14) ;
}
int main(int argc, char* argv[]) {
    char command[BUFF_SIZE] ;
    while(1)
    {
        write(1 , "$ sdhfkja" , 2) ;
        int len = read(0 , command , BUFF_SIZE) ;
        command[len-1] = '\0' ;   // -1 to ommite the next line character ie. ENTER
        if( strcmp( command , "exit" ) == 0 )    break ;
        // if( strcmp( command , "clear") == 0)     printf("\e[1;1H\e[2J");
        // process_command( command , argv ) ;
        argc = update_arguments(command, argv) ;
        execute(argv , argc) ;
    }
    return 0 ;
}
