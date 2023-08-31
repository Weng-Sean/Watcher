#include <stdlib.h>
#include <stdio.h>
#include <string.h>



void request_bitstamp(char* ticker){
    char command[1024];

    // build command to run uwsc and subscribe to the live_orders_btcusd channel
    // sprintf(command, "uwsc -q wss://ws.bitstamp.net | grep -v -o0 '{\"event\":\"bts:subscription_succeeded\",\"channel\":\"live_trades_%s\",\"data\":{}}' --line-buffered", ticker);
    sprintf(command, "uwsc -q wss://ws.bitstamp.net");



    // execute command and read output
    FILE* fp = popen(command, "w");
    if (!fp) {
        perror("popen failed");
        exit(1);
    }

    char subscribe[1024];
    // sprintf(subscribe, "{ \"event\": \"bts:subscribe\", \"data\": { \"channel\": \"live_trades_%s\" } }\n", ticker);
    sprintf(subscribe, "{ \"event\": \"bts:subscribe\", \"data\": { \"channel\": \"live_orders_%s\" } }\n", ticker);

    
    fprintf(fp, "%s\n", subscribe);


    // quit
    // sprintf(subscribe, "!q");
    // fprintf(fp, "%s\n", subscribe);


    // // read output from uwsc program
    // char msg[1024];

    // while (fgets(msg, 1024, fp))
    // {
    //     printf("1234");
    //     // remove trailing newline character
    //     msg[strcspn(msg, "\n")] = 0;

    //     // print server message
    //     // check if msg contains "data" key and if it's not empty
    //     if (strstr(msg, "\"data\":{") != NULL && strcmp(strstr(msg, "\"data\":{") + 8, "}") != 0) {
    //         printf("%s\n", msg);
    //     }
    //     printf("%s\n", msg);
        
    // }
    // close file pointer
    pclose(fp);
}



// API 2

//     FILE* fp;
//     char output[1024];
//     char* command = "echo { \\\"event\\\": \\\"bts:subscribe\\\", \\\"data\\\": { \\\"channel\\\": \\\"live_trades_btcusd\\\" } } | uwsc wss://ws.bitstamp.net";
//     fp = popen(command, "r");
//     if (fp == NULL) {
//         printf("Failed to execute command\n");
//         return 1;
//     }
//     while (fgets(output, sizeof(output), fp) != NULL) {
//         printf("%s", output);
//     }
//     pclose(fp);
//     return 0;


