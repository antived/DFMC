#include <thread>
#include "metadata_common.h"

int main(){
    parser();
    std::thread server_thread(run_server);
    server_thread.join(); //idea is to keep the main thread to br running inf, until the server_thread is done.
    return 0; 
}


