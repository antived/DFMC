#include <iostream>
#include <string.h>
#include <map>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <ios>
#include <fstream>
#include <unordered_map>
#include <cstring>    
#include <chrono>
#include <thread>
#include <sstream>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "yaml-cpp/yaml.h"
#include <pqxx/pqxx>
#include "common_data.h"
#include "httplib.h"
#include <thread>
#include <pqxx/pqxx>
#include <api/endpoints.h>


using namespace std;
using namespace httplib;

void incoming_machine_data(httplib::Server&);
void incoming_api(httplib::Server&);

int main(){
    Server central_server;
    incoming_machine_data(central_server);
    incoming_api(central_server);
    YAML::Node config_file = YAML::LoadFile("/home/vedant/cpp_dev/central_server/config.yaml");
    int central_port = config_file["port"].as<int>();
    std::thread server_thread([&central_server, central_port]() {
        central_server.listen("0.0.0.0", central_port);
    });
    std::thread cli_thread([]() {
        run_user_cli();
    });
    server_thread.join();
    cli_thread.join();
    return 0;
}




