#ifndef COMMON_DATA_H
#define COMMON_DATA_H

#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <string>
#include <httplib.h>


class uuid_machine{
    public:
        std::string machine_no;
        std::string machine_ip;
        int machine_port;

        uuid_machine() {}
        uuid_machine(const std::string& no, const std::string& ip, int port)
            : machine_no(no), machine_ip(ip), machine_port(port){}

};

inline std::unordered_map<std::string,uuid_machine> uuid_mac;
inline std::unordered_map<int,std::string> no_uuid;
inline std::unordered_set<int> machines_found;
inline std::mutex central_mutex;

void incoming_machine_data(httplib::Server& server);
void incoming_api(httplib::Server& server);

#endif