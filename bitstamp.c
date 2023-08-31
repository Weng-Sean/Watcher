#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>

#include "ticker.h"
#include "watcher.h"
#include "bitstamp.h"
#include "debug.h"
#include <signal.h>
#include <sys/time.h>
#include "argo.h"
#include "store.h"

extern void request_bitstamp();



// FIX VOLUME


/*
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


void child_sigio_handler(int signo) {
    debug("Received SIGIO signal from child process\n");
}

WATCHER *bitstamp_watcher_start(WATCHER_TYPE *type, char *args[])
{

    
    int pipefd[2];
    pipe(pipefd);

    int pipefd2[2];
    pipe(pipefd2);

    WATCHER *w = malloc(sizeof(WATCHER));
    w->read_file_descriptor = pipefd[0];
    debug("read: %d\n", pipefd[0]);
    w->write_file_descriptor = pipefd2[1];
    debug("write: %d\n", pipefd2[1]);
    
    w->type = type; 
    // Allocate memory for w->name
    w->name = malloc(strlen(args[0]) + 1); // add 1 for null terminator

    // Copy contents of args[0] to w->name
    strcpy(w->name, args[0]);

    // Allocate memory for w->tracking_type
    w->tracking_type = malloc(strlen(args[1]) + 1); // add 1 for null terminator

    // Copy contents of args[1] to w->tracking_type
    strcpy(w->tracking_type, args[1]);

    w->watcher_args = malloc(strlen(args[2]) + 1);
    strcpy(w->watcher_args, args[2]);

    w->type = type;
    w->trace = 0;
    w->serial = 0;

    // Set the process as the owner of the standard input file descriptor
    fcntl(pipefd[0], F_SETOWN, getpid());

    // Enable SIGIO signals for the standard input file descriptor
    fcntl(pipefd[0], F_SETFL, O_ASYNC | O_NONBLOCK);

    fcntl(pipefd[0], F_SETSIG, SIGIO);



    // if (getpid() == PID)
    // {
    //     debug("forking\n");
        
        if ((w->process = fork()) == 0)
        {
            // Child process
            close(pipefd[0]); // Close the read end of the pipe
            close(pipefd2[1]); // Close the read end of the pipe
            // Initialization code here

            dup2(pipefd2[0], STDIN_FILENO);
            dup2(pipefd[1], STDOUT_FILENO);

            debug("size of watcher: %ld\n", sizeof(*w));
            
            
            // populate w
            
            execvp(type->argv[0], type->argv);
            // exit(EXIT_SUCCESS);
            
            char buffer[1024];
            read(pipefd2[0], buffer, 1024);

        }
        else
        {
            // Parent process
            close(pipefd[1]); // Close the write end of the pipe
            close(pipefd2[0]); // Close the read end of the pipe


            write(pipefd2[1], "{ \"event\": \"bts:subscribe\", \"data\": { \"channel\": \"", 50);
            write(pipefd2[1], w->tracking_type, strlen(w->tracking_type));
            write(pipefd2[1], "\" } }\n", 6);

            debug("bitstamp return wp"); 
            
        }
        return w;
    

}


int bitstamp_watcher_stop(WATCHER *wp)
{
    if (strcmp(wp->type->name, "CLI") == 0)
    {
        debug("ATTEMPTING TO STOP CLI\n");
        return 1;
    }
    wp->prev->next = wp->next;
    wp->next->prev = wp->prev;
    free(wp->name);
    free(wp->tracking_type);
    free(wp);
    kill(wp->process, SIGTERM);
    return 0;
}

int bitstamp_watcher_send(WATCHER *wp, void *arg)
{
    write(wp->write_file_descriptor, arg, strlen(arg));

    return 0;

}

int bitstamp_watcher_recv(WATCHER *wp, char *txt)
{
    // printf("enter recv method\n");
    // printf("trace: %d", wp->trace);

    wp -> serial++;
    if (wp->trace){
        // printf("enter here\n");
        // time_t t;
        // time(&t);
        struct timeval tv;
        gettimeofday(&tv, NULL);

        // long long milliseconds = (tv.tv_sec * 1000LL) + (tv.tv_usec / 1000LL);

        // char msg[1024];
        // sprintf(msg, "[%ld.%06ld][%-10s][%2d][%5ld]:%s\n\n", tv.tv_sec,tv.tv_usec,"CLI", wp->read_file_descriptor, wp->serial++, txt);
        fprintf(stderr, "[%ld.%06ld][%-10s][%2d][%5ld]: > %s\n", tv.tv_sec,tv.tv_usec,wp->type->name, wp->read_file_descriptor, wp->serial, txt);
        // cli_watcher_send(wp, msg);
    }


    char string_start[] = "\b\bServer message: '";
    // printf("%s\n", txt);
    // fflush(stdout);
    if (strncmp(txt, string_start, 19) == 0){
        // printf("%d\n", __LINE__);
        // fflush(stdout);
        
        if (txt[strlen(txt) - 2] == '\''){
            // printf("enter: %s\n", txt);
            
            int json_len = strlen(txt) - strlen(string_start);
            char json_buffer[json_len];

            strcpy(json_buffer, txt + strlen(string_start));

            // printf("%s\n", json_buffer);

            // printf("%d\n", __LINE__);
            // fflush(stdout);

            json_buffer[strlen(json_buffer) - 2] = 0;
            


            FILE* f = fmemopen(json_buffer, strlen(json_buffer), "r");
            ARGO_VALUE *av = argo_read_value(f);

            // printf("%d\n", __LINE__);
            // fflush(stdout);


            ARGO_VALUE *event = argo_value_get_member(av, "event");
            char* value = argo_value_get_chars(event);
            // printf("%d\n", __LINE__);
            // fflush(stdout);
            if (strcmp(value, "trade") == 0){
                // printf("%d\n", __LINE__);
                // fflush(stdout);

                ARGO_VALUE* data = argo_value_get_member(av, "data");
                ARGO_VALUE *price = argo_value_get_member(data, "price");

                double p = 0;
                argo_value_get_double(price, &p);

                char key_price[strlen(wp->type->name) + strlen(wp-> tracking_type) + 8];
                sprintf(key_price, "%s:%s:%s", wp->type->name, wp->tracking_type, "price");
                key_price[sizeof(key_price) - 1] = 0;

                // printf("key: %s\n", key_price);
                // fflush(stdout);

                struct store_value store_p;
                store_p.type = STORE_DOUBLE_TYPE;
                store_p.content.double_value = p;

                store_put(key_price, &store_p);

                



                // printf("%d\n", __LINE__);
                // fflush(stdout);

                ARGO_VALUE *amount = argo_value_get_member(data, "amount");

                double a = 0;
                argo_value_get_double(amount, &a);

                char key_amount[strlen(wp->type->name) + strlen(wp-> tracking_type) + 9];
                sprintf(key_amount, "%s:%s:%s", wp->type->name, wp->tracking_type, "amount");
                key_amount[sizeof(key_amount) - 1] = 0;

                struct store_value store_a;
                store_a.type = STORE_DOUBLE_TYPE;
                store_a.content.double_value = a;

                store_put(key_amount, &store_a);

                // double v = 0;

                // char key_volume[strlen(wp->type->name) + strlen(wp-> tracking_type) + 9];
                // sprintf(key_volume, "%s:%s:%s", wp->type->name, wp->tracking_type, "volume");
                // key_volume[sizeof(key_volume) - 1] = 0;

                // struct store_value store;
                // store.type = STORE_DOUBLE_TYPE;
                // store.content.double_value = v;

                // store_put(key_volume, &store);


                char key_vol[strlen(wp->type->name) + strlen(wp-> tracking_type) + 9];
                sprintf(key_vol, "%s:%s:%s", wp->type->name, wp->tracking_type, "volume");
                key_vol[sizeof(key_vol) - 1] = 0;

                struct store_value* vp = store_get(key_vol);
                struct store_value vol_store;
                vol_store.type = STORE_DOUBLE_TYPE;
                if (vp == NULL){
                    vol_store.content.double_value = a;

                    store_put(key_vol, &vol_store);
                }
                else{

                    vol_store.content.double_value = vp->content.double_value + a;
                    store_put(key_vol, &vol_store);

                    store_free_value(vp);
                }

            }

            // printf("%d\n", __LINE__);
            // fflush(stdout);
            
            free(value);
            fclose(f);
            argo_free_value(av);




        }
    }

    return 0;
}

int bitstamp_watcher_trace(WATCHER *wp, int enable)
{
    wp->trace = enable;
    // printf("trace: %d\n",enable);
    return 0;
}
