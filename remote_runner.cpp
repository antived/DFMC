#include <thread>
#include <string>
#include <iostream>
#include "metadata_common.h"

int main(int argc, char *argv[]){
    parser();
    if(argc < 2 ){
        std::cerr << "Usage: ./remote_runner <config_file_path>\n";
        return 1;
    }
    std::string config_path = argv[1];
    std::thread server_thread([&config_path](){
        run_server(config_path);
    });
    server_thread.join(); //idea is to keep the main thread to br running inf, until the server_thread is done.
    return 0; 
}


