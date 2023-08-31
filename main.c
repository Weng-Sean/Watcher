#include <stdlib.h>
#include <unistd.h>
#include "store.h"
#include "ticker.h"
#include <stdio.h>
#include <string.h>

extern void request_bitstamp(char* ticker);

int main(int argc, char *argv[]) {

    // char* args[] = { "uwsc", "wss://ws.bitstamp.net", NULL };
    // execvp(args[0], args);
    // perror("execvp"); // only runs if there was an error

    if(ticker())
	return EXIT_FAILURE;
    else
	return EXIT_SUCCESS;
    

    // request_bitstamp("dogeusd");
    // request_bitstamp("btcusd");
    // request_bitstamp("ethusd");

    


}
