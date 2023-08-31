#include <stdlib.h>

#include "ticker.h"
#include "watcher.h"
#include "store.h"
#include "cli.h"
#include "debug.h"
#include <unistd.h>
#include "string.h"
#include <time.h>
#include <sys/time.h>
#include <stdio.h>


#define PRINT_ERROR cli_watcher_send(wp, "???\n")

#define MAX_TOKEN_LEN 128

/*
WATCHER_TYPE
Each instance of the watcher_type structure defines something roughly
analogous to what a "class" would be used for in an object-oriented language.
The field name is a "class variable" that gives the name of the watcher type.
The field argv provides the name of an executable (in argv[0]) and additional
arguments to pass to that executable via execvp().
The remaining fields are pointers to "methods".
There is a start method, which is in essence a constructor for initializing
an instance of the watcher type.
The stop method is used to terminate a previously constructed instance.
The send method is used to output a command or other information to the watcher
instance.
The recv method is used to input asynchronous data from the watcher and act on it.
Finally the trace method is used to enable or disable tracing of asynchronous
messages received from the watcher instance.
*/

WATCHER *cli_watcher = NULL;

WATCHER *cli_watcher_start(WATCHER_TYPE *type, char *args[])
{
    type->argv = watcher_types[CLI_WATCHER_TYPE].argv;
    type->name = watcher_types[CLI_WATCHER_TYPE].name;
    type->stop = watcher_types[CLI_WATCHER_TYPE].stop;
    type->send = watcher_types[CLI_WATCHER_TYPE].send;
    type->recv = watcher_types[CLI_WATCHER_TYPE].recv;
    type->trace = watcher_types[CLI_WATCHER_TYPE].trace;

    //     struct watcher{
    //     int socket_fd;          // File descriptor for the socket
    //     WATCHER_TYPE *type;     // Pointer to the watcher's type
    //     void *data;             // Pointer to any extra data needed by the watcher
    //     struct addrinfo *addr;  // Pointer to the server's address info
    //     int running;            // Flag indicating if the watcher is running

    //     // bitstamp.net(404756,3,6)
    //     long process; // 404756
    //     int file_descriptor; // 3
    //     int write_file_descriptor; //6
    // };

    WATCHER *w = malloc(sizeof(WATCHER));
    w->process = -1;
    w->read_file_descriptor = 0;
    w->write_file_descriptor = 1;
    w->type = type;
    w->index = 0;
    w->name = "CLI";
    w->next = w;
    w->prev = w;
    w->serial = 0;
    w->trace = 0;


    return w;
}

int cli_watcher_stop(WATCHER *wp)
{
    WATCHER *iter = cli_watcher->next;

    while(iter != cli_watcher){
        iter = iter->next;
        iter->prev->type->stop(iter->prev);
    }
    return 0;
}

int cli_watcher_send(WATCHER *wp, void *arg)
{
    write(wp->write_file_descriptor, arg, strlen(arg));

    return 0;
}

int cli_watcher_recv(WATCHER *wp, char *txt)
{
    // debug("COMMAND RECEIVED: %s\n", txt);
    wp->serial++;
    char command[1024];
    strcpy(command, txt);
   
    int i = 0;
    // new line char
    while (*(txt + i) != 10 && *(txt + i) != 0)
    {
        debug("%c\n", *(txt + i));
        debug("%d\n", *(txt + i));
        i++;
    }
    // printf("%d\n", i);
    if (i == 0)
    {
        PRINT_ERROR;
        return 0;
    }
    *(txt + i) = 0;
    
    if (wp->trace)
    {
        // time_t t;
        // time(&t);
        struct timeval tv;
        gettimeofday(&tv, NULL);

        // long long milliseconds = (tv.tv_sec * 1000LL) + (tv.tv_usec / 1000LL);

        char msg[100];
        sprintf(msg, "[%ld.%06ld][%-10s][%2d][%5ld]: %s\n\n", tv.tv_sec,tv.tv_usec,"CLI", wp->read_file_descriptor, wp->serial, txt);
        fprintf(stderr, "%s", msg);
        // cli_watcher_send(wp, msg);
    }

    char *token1 = NULL;
    char *token2 = NULL;
    char *token3 = NULL;
    char *delim = " \t\n"; // whitespace delimiters
    char *current_token;

    current_token = strtok(txt, delim);

    while (current_token != NULL)
    {
        if (token1 == NULL)
        {
            debug("Token1: %s\n", current_token);
            token1 = current_token;
            current_token = strtok(NULL, delim);
        }
        else if (token2 == NULL)
        {
            debug("Token2: %s\n", current_token);
            debug("Token2: %d\n", *current_token);
            token2 = current_token;
            current_token = strtok(NULL, delim);
        }
        else if (token3 == NULL)
        {
            debug("Token3: %s\n", current_token);
            token3 = current_token;
            current_token = strtok(NULL, delim);
        }
        else
        {
            break;
        }
    }

    if (strcmp(token1, "start") == 0 && token2 != NULL && token3 != NULL)
    {
        // ticker> start bitstamp.net live_trades_btcgbp
        char* watcher_args = command;
        int space_cnt = 0;

        while (space_cnt != 2){

            
            if (*watcher_args == 32){
                space_cnt++;
            } 
            watcher_args++;
            debug("%s", watcher_args); 
        }
        
        for (int i = 0; *(watcher_args+i) != 0; i++){
            if (*(watcher_args+i) == 10){
                *(watcher_args+i) = 0;
                break;
            }
        }
        


        char *args[] = {token2, token3, watcher_args};
        WATCHER *w;

        int i = 0;
        while (watcher_types[i].name != NULL)
        {
            // printf("%s\n", token2);
            // printf("%s\n", watcher_types[i].name);

            if (strcmp(token2, watcher_types[i].name) == 0)
            {
                // printf("%d\n", __LINE__);

                w = watcher_types[i].start(&watcher_types[i], args);
                // printf("%d\n", __LINE__);

                break;
            }
            i++;
        }
            // printf("%d\n", __LINE__);

        if (watcher_types[i].name == NULL)
        {
            PRINT_ERROR;
        }
        else
        {

            if (cli_watcher->next == NULL)
            {
                cli_watcher->next = w;
                cli_watcher->prev = w;
                w->next = cli_watcher;
                w->prev = cli_watcher;
            }
            else
            {
                int i = 1;
                struct watcher *iter = cli_watcher->next;
                while (iter != cli_watcher && iter->index == i++)
                {
                    iter = iter->next;
                }
                if (iter == cli_watcher)
                {
                    cli_watcher->prev->next = w;
                    w->prev = cli_watcher->prev;
                    w->next = cli_watcher;
                    cli_watcher->prev = w;
                }
                else
                {
                    iter->prev->next = w;
                    w->prev = iter->prev;
                    iter->prev = w;
                    w->next = iter;
                }
            }
            w->index = w->prev->index + 1;


            // printf("%d\n", __LINE__);
        }

        // struct watcher *iter = cli_watcher->next;

        // while (iter != cli_watcher)
        // {
        // if (iter->running)
        // {

        //     printf("%d\t%s(%ld, %d,%d)\n",
        //            iter->index, iter->name, iter->process, iter->file_descriptor, iter->write_file_descriptor);
        // }
        // debug("%s\n", iter->name);
        // debug("%p\n", iter->name);
        // iter = iter->next;
        // }
    }
    else if (strcmp(token1, "stop") == 0 && token3 == NULL)
    {
        // Tracing is turned off for watcher #1.
        // ticker> stop 1

        if (token2 == 0)
        {
            PRINT_ERROR;
        }
        else
        {
            struct watcher *iter = cli_watcher->next;

            while (iter != cli_watcher)
            {
                if (iter->index == atoi(token2))
                {
                    // if (iter->running)
                    // {
                    iter->type->stop(iter);
                    break;
                    // }
                    // else
                    // {
                    //     PRINT_ERROR;
                    // }
                }
                else if (iter->index > atoi(token2))
                {
                    // printf("%d,2\n", getpid());

                    PRINT_ERROR;
                    break;
                }
                iter = iter->next;
            }

            if (iter == cli_watcher){
                PRINT_ERROR;
            }
        }
    }
    else if (strcmp(token1, "trace") == 0 && token2 != NULL && token3 == NULL)
    {
        int token2_as_int = atoi(token2);
        if (token2_as_int == 0)
        {
            if (strcmp(token2, "0") == 0){
            debug("tracing");
            cli_watcher_trace(wp, 1);
            }
            else{
                PRINT_ERROR;
            }
        }
        else
        {
            // printf("enter else %d\n", token2_as_int);
            WATCHER *iter = cli_watcher->next;


            while (iter != cli_watcher){
                if (iter->index == token2_as_int){
                    iter->type->trace(iter, 1);
                    // printf("iter index: %d\n", iter->index);
                    break;
                }
                iter = iter->next;
            }

            if (iter == cli_watcher){
                PRINT_ERROR;
            }
            
        }
        // ticker> trace 1 
    }
    else if (strcmp(token1, "untrace") == 0 && token3 == NULL)
    {   
       if (atoi(token2) == 0)
        {
            if (strcmp(token2, "0") == 0){
                debug("tracing");
                cli_watcher_trace(wp, 0);
            }
            else{
                PRINT_ERROR;
            }
        }
        else
        {
            WATCHER *iter = cli_watcher->next;
            while (iter != cli_watcher){
                if (iter->index == atoi(token2)){
                    iter->type->trace(iter, 0);
                    break;
                }
                iter = iter->next;
            }
        }
        // ticker> untrace 1
    }
    else if (strcmp(token1, "watchers") == 0 && token2 == NULL && token3 == NULL)
    {
        // ticker> watchers
        cli_watcher_send(wp, "0\tCLI(-1,0,1)\n");
        struct watcher *iter = cli_watcher->next;

        while (iter != cli_watcher)
        {
            
            // if (iter->running)
            // {

            // TODO
            int buffer_size = 1024;
            char temp_buffer[buffer_size];



            sprintf(temp_buffer, "%d\t%s(%ld,%d,%d) %s %s [%s]\n",
                    iter->index, iter->name, iter->process, iter->read_file_descriptor, 
                    iter->write_file_descriptor, iter->type->argv[0], iter->type->argv[1],iter->watcher_args);
            cli_watcher_send(wp, temp_buffer);
            // }
            iter = iter->next;
        }
    }
    else if (strcmp(token1, "show") == 0 && token2 != NULL && token3 == NULL)
    {
        // ticker> show bitstamp.net:live_trades_btcusd:price
        debug("SHOWING");

        // printf("token2: %s", token2);
        // fflush(stdout);

        // int i = 0;
        // char* iter = token2;
        // while(*(i+iter) != 0){
            // printf("%c\n", *(i+iter));
            // printf("%d\n", *(i+iter));

            // i++;
        // }
        // if (strcmp(token2, "bitstamp.net:live_trades_btcusd:price") == 0){
        //     printf("equal!!!\n");
        //     fflush(stdout);

        int length = strlen(token2);
        char* string = malloc(length + 1);
        for(int i = 0; i < length; i++)
        {
            string[i] = token2[i];
        }
        string[length] = 0;


        
        // else{
        //     printf("NOT equal!!!\n");
        //     fflush(stdout);
        // }
        struct store_value* sv = store_get(string);
        if (sv == NULL){
            PRINT_ERROR;
        }
        else{
            switch(sv->type){
                case STORE_INT_TYPE:
                    fprintf(stdout, "%s\t%d\n", string, sv->content.int_value);
                    break;
                case STORE_LONG_TYPE:
                    fprintf(stdout, "%s\t%ld\n", string, sv->content.long_value);

                    break;
                case STORE_DOUBLE_TYPE:
                    fprintf(stdout, "%s\t%f\n", string, sv->content.double_value);

                    break;
                case STORE_STRING_TYPE:
                    fprintf(stdout, "%s\t%s\n", string, sv->content.string_value);

                    break;

                default:
                    break;
            }
            store_free_value(sv);
        }

    }
    else if (strcmp(token1, "quit") == 0 && token2 == NULL && token3 == NULL)
    {
        // ticker> quit
        debug("QUITTING\n");
        cli_watcher_stop(wp);
        return 1;
    }
    else
    {
        PRINT_ERROR;
    }

    return 0;
}

int cli_watcher_trace(WATCHER *wp, int enable)
{
    wp->trace = enable;
    return 0;
}
