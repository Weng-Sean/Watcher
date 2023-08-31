#include <stdlib.h>
#include "ticker.h"
#include "watcher.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "debug.h"
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024
extern void request_bitstamp(char *ticker);
extern pid_t PID;
// request_bitstamp("dogeusd");

volatile int trace_fd = -1;

volatile int is_command_input = 0;

void sigio_handler(int signal, siginfo_t *info, void *ucontext)
{

    // Return type of the handler function should be void
    if (info->si_fd == STDIN_FILENO){
        is_command_input = 1;
    }
    else{
        trace_fd = info->si_fd;
    }
    debug("\nInside handler function\n");



}

void sigchild_handler(int signal, siginfo_t *info, void *ucontext)
{

    // Return type of the handler function should be void
    int n;
    waitpid(info->si_pid, &n, 0);

}

int ticker(void)
{

    // Install the signal handler for SIGIO
    struct sigaction sa;
    sa.sa_sigaction = sigio_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    struct sigaction sa2;
    sa2.sa_sigaction = sigchild_handler;
    sa2.sa_flags = SA_SIGINFO;

    sigaction(SIGIO, &sa, NULL);
    sigaction(SIGCHLD, &sa2, NULL);

    // Set the process as the owner of the standard input file descriptor
    fcntl(STDIN_FILENO, F_SETOWN, getpid());

    // Enable SIGIO signals for the standard input file descriptor
    fcntl(STDIN_FILENO, F_SETFL, O_ASYNC | O_NONBLOCK);

    fcntl(STDIN_FILENO, F_SETSIG, SIGIO);

    // if (sigaction(SIGIO, &sa, NULL) == -1) {
    // perror("sigaction");
    // return 1;
    // }

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);

    cli_watcher = watcher_types[CLI_WATCHER_TYPE].start(&watcher_types[CLI_WATCHER_TYPE], NULL);
    char buffer[BUFFER_SIZE];
    watcher_types[CLI_WATCHER_TYPE].send(cli_watcher, "ticker> ");

    while (1)
    {
        // prompt the user for the next command
        
        ssize_t size_read = read(STDIN_FILENO, buffer, sizeof(buffer));
        // if a file
        // printf("%ld\n", size_read);
        if (size_read >= 0)
        {  
            if (size_read == 0)
            {
                return 0;
            }
            else
            {
                
                char* char_iter = buffer;
                char* end = buffer + strlen(buffer); // point to \0

                while (char_iter < end){
                    if (*char_iter == 10){
                        *char_iter = 0;
                    }
                    char_iter++;
                }

                // char *delim = "\n";
                // // printf("%s\n",buffer);

                // char *current_command = strtok(buffer, delim);
                char* current_command = buffer;

                while (strlen(current_command))
                {
                    char *command = current_command;

                    // printf("\n--%s\n", command);
                   
                    if (watcher_types[CLI_WATCHER_TYPE].recv(cli_watcher, command) != 0)
                    {
                        break;
                    };
                    // printf("0\n"); 
                    // current_command = strtok(NULL, delim);
                    // printf("1\n");
                    watcher_types[CLI_WATCHER_TYPE].send(cli_watcher, "ticker> ");
                    // printf("2\n");
                    current_command += strlen(current_command) + 1;

                }
                watcher_types[CLI_WATCHER_TYPE].send(cli_watcher, "ticker> ");
                return 0;
            }
        }
        else
        {

            // Suspend the program until SIGIO is received
            // sigset_t sigs_to_wait;
            // sigemptyset(&sigs_to_wait);

            // sigaddset(&sigs_to_wait, SIGIO);

            // // Unblock SIGIO temporarily to avoid a race condition
            // sigset_t orig_mask;
            // sigprocmask(SIG_UNBLOCK, &sigs_to_wait, &orig_mask);

            // if (sigsuspend(&orig_mask) == -1 && errno != EINTR)
            // {
            //     perror("sigsuspend");
            //     exit(1);
            // }
            sigsuspend(&mask);

            if (trace_fd >= 0){
                WATCHER* iter = cli_watcher->next;
                while (iter != cli_watcher){
                    // printf("read fd: %d\n", )
                    if (iter->read_file_descriptor == trace_fd){
                        // printf("trace fd: %d\n", trace_fd);

                        char buf[1024];
                        int size_read = read(trace_fd, buf, 1023);
                        buf[size_read-2] = 0;


                        
                        iter->type->recv(iter, buf);
                        // watcher_types[CLI_WATCHER_TYPE].send(cli_watcher, "ticker> ");
                        
                        break;
                    }
                    iter = iter->next;
                }
                trace_fd = -1;
            }

            if (is_command_input == 0){
                continue;
            }
            else{
                is_command_input = 0;
            }

            // Re-block SIGIO before accessing shared resources
            // sigprocmask(SIG_BLOCK, &sigs_to_wait, NULL);

            // Read the user input

            // fgets(buffer, sizeof(buffer), stdin);

            read(STDIN_FILENO, buffer, sizeof(buffer));

            // send to the CLI watcher
            debug("%s", buffer);
            if (watcher_types[CLI_WATCHER_TYPE].recv(cli_watcher, buffer) != 0)
            {
                break;
            }
            watcher_types[CLI_WATCHER_TYPE].send(cli_watcher, "ticker> ");
                        
        }
    }
    watcher_types[CLI_WATCHER_TYPE].send(cli_watcher, "ticker> ");
    return 0;
}

// int ticker(void)
// {
//     cli_watcher = cli_watcher_start(&watcher_types[0], NULL);

//     char command[MAX_TOKEN_LEN * 3];
//     char *token1 = NULL;
//     char *token2 = NULL;
//     char *token3 = NULL;
//     char *delim = " \t\n"; // whitespace delimiters
//     char *current_token;
//     printf("ticker> ");

//     while (fgets(command, sizeof(command), stdin))
//     {
//         current_token = strtok(command, delim);

//         while (current_token != NULL)
//         {
//             if (token1 == NULL)
//             {
//                 // printf("Token1: %s\n", current_token);
//                 token1 = current_token;
//                 current_token = strtok(NULL, delim);
//             }
//             else if (token2 == NULL)
//             {
//                 // printf("Token2: %s\n", current_token);
//                 token2 = current_token;
//                 current_token = strtok(NULL, delim);
//             }
//             else if (token3 == NULL)
//             {
//                 // printf("Token3: %s\n", current_token);
//                 token3 = current_token;
//                 current_token = strtok(NULL, delim);
//             }
//             else{
//                 break;
//             }
//         }

//         // remove the newline character from the end of the string
//         // command[strcspn(command, "\n")] = 0;

//         // handle the command
//         if (strcmp(token1, "start") == 0 && token2 != NULL && token3 != NULL)
//         {
//             // ticker> start bitstamp.net live_trades_btcusd
//             // ticker> start bitstamp.net live_trades_btcgbp
//             char *args[] = {token2, token3};
//             WATCHER *w = bitstamp_watcher_start(&watcher_types[1], args);
//             if (cli_watcher->next == NULL)
//             {
//                 cli_watcher->next = w;
//                 cli_watcher->prev = w;
//                 w->next = cli_watcher;
//                 w->prev = cli_watcher;
//             }
//             else
//             {
//                 cli_watcher->prev->next = w;
//                 w->prev = cli_watcher->prev;
//                 w->next = cli_watcher;
//                 cli_watcher->prev = w;
//             }
//             w->index = w->prev->index + 1;

//             struct watcher *iter = cli_watcher->next;

//             while (iter != cli_watcher)
//             {
//                 // if (iter->running)
//                 // {

//                 //     printf("%d\t%s(%ld, %d,%d)\n",
//                 //            iter->index, iter->name, iter->process, iter->file_descriptor, iter->write_file_descriptor);
//                 // }
//                 debug("%s\n", iter->name);
//                 debug("%p\n", iter->name);
//                 iter = iter->next;
//             }
//         }
//         else if (strcmp(token1, "stop") == 0 && token3 == NULL)
//         {
//             // Tracing is turned off for watcher #1.
//             // ticker> stop 1

//             if (token2 == 0)
//             {
//                 PRINT_ERROR;
//             }
//             else
//             {
//                 struct watcher *iter = cli_watcher->next;

//                 while (iter != cli_watcher)
//                 {
//                     if (iter->index == atoi(token2))
//                     {
//                         if (iter->running){
//                         iter->type->stop(iter);
//                         break;
//                         }
//                         else{
//                             PRINT_ERROR;
//                         }
//                     }
//                     else if (iter->index > atoi(token2))
//                     {
//                         // printf("%d,2\n", getpid());

//                         PRINT_ERROR;
//                         break;
//                     }
//                     iter = iter->next;
//                 }
//             }
//         }
//         else if (strcmp(token1, "trace") == 0 && token3 == NULL)
//         {
//             // ticker> trace 1
//         }
//         else if (strcmp(token1, "untrace") == 0 && token3 == NULL)
//         {
//             // ticker> untrace 1
//         }
//         else if (strcmp(token1, "watchers") == 0 && token2 == NULL && token3 == NULL)
//         {
//             // ticker> watchers
//             printf("0\tCLI(-1,0,1)\n");
//             struct watcher *iter = cli_watcher->next;

//             while (iter != cli_watcher)
//             {
//                 if (iter->running)
//                 {

//                     // TODO
//                     printf("%d\t%s(%ld, %d,%d)\n",
//                            iter->index, iter->name, iter->process, iter->read_file_descriptor, iter->write_file_descriptor);
//                 }
//                 iter = iter->next;
//             }
//         }
//         else if (strcmp(token1, "quit") == 0 && token2 == NULL && token3 == NULL)
//         {
//             // ticker> quit
//             break;
//         }
//         else
//         {
//             PRINT_ERROR;
//         }

//         // reset
//         token1 = token2 = token3 = NULL;

//         // prompt the user for the next command
//         printf("ticker> ");
//     }

//     debug("pid: %d\n", getpid());

//     return 0;
// }
