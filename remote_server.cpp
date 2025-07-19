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
#include <queue>        
#include "httplib.h"
#include "metadata_common.h"
#include <chrono>
#include <thread>
#include <sstream>

using namespace std;
using namespace httplib;

//the idea is to run this code on a remoter server, and set up http server on the machine that runs this
//and parser_with_yaml. It will query incoming requests and also periodically post data to the central cache server.
//the metadata_common is storing the global map for the path, meta data.
//Set up http server that is going to going to service REST API based query requests.
void push_metadata(){
    while(true){
        this_thread::sleep_for(chrono::minutes(25));
        std::ifstream ipstream("metadata.json");
        if(!ipstream) continue;
        stringstream buffer;
        buffer << ipstream.rdbuf();
        ipstream.close();
        //sending it to the central cache server.
        Client cli("http://192.168.1.100:8080");     ////IMP -> change this to that of the actual central server.
        auto res = cli.Post("/metadata_full",buffer.str(),"application/json");
        if(res && res->status == 200){
            cout << "[$INFO$]The metadata.json file was sent successfully to the central node" << endl;
        }
        else{
            cerr << "[$ERROR$]The metadata.json file could not be sent to central node" << endl;
        }
    }
}

void run_server(){
    thread t(push_metadata);
    t.detach();
    Server svr;
    svr.Get("/file_size",[](const Request& req, Response& resp){
        //the incoming GET request is going to be of the form ../file_size?path=home/vedant...
        auto it = req.get_param_value("path"); //path is the key, the URL extension is the value.
        int status_code;
        if(it.empty()){
            status_code = 400;
            resp.status = status_code;
            resp.set_content("Missing 'path' query param","text/plain");
            return;
        }
        std::filesystem::path fin_path = it;
        if(mapp.find(fin_path) != mapp.end()){
            int file_size= mapp[fin_path].Size;
            status_code = 200;  //this one is for OK.
            resp.status = status_code; 
            resp.set_content("the size of the file is:"+ to_string(file_size),"text/plain");
        }
        else{
            status_code = 404; //not found.
            resp.status = status_code;
            resp.set_content("The file with the path could not be found in the directoy :"+
                    (string)base_dir,"text/plain");
        }
    });
    svr.Get("/last_time",[](const Request& req, Response &resp){
        auto it = req.get_param_value("path");
        int status_code = 0;
        if(it.empty()){
            status_code = 400;
            resp.status = status_code;
            resp.set_content("Missing 'path' query param","text/plain");
            return;
        }
        std::filesystem::path fin_path = it;
        if(mapp.find(fin_path) != mapp.end()){
            std::filesystem::file_time_type file_time = mapp[fin_path].last_time;
            auto i = chrono::time_point_cast<std::chrono::system_clock::duration>(
                file_time - filesystem::file_time_type::clock::now()
                    + chrono::system_clock::now()
            );
            time_t tt = chrono::system_clock::to_time_t(i);
            string time_str = asctime(localtime(&tt));
            time_str.pop_back(); //just to remove the '\n'
            status_code = 200;  //this one is for OK.
            resp.status = status_code; 
            resp.set_content("the last time of change of the file is:"+time_str,"text/plain");
        }
        else{
            status_code = 404; //not found.
            resp.status = status_code;
            resp.set_content("The file with the path could not be found in the directoy :"+
                    (string)base_dir,"text/plain");
        }
    });
    svr.Get("/name",[](const Request &req, Response &resp){
        auto it = req.get_param_value("path"); //path is the key, the URL extension is the value.
        int status_code;
        if(it.empty()){
            status_code = 400;
            resp.status = status_code;
            resp.set_content("Missing 'path' query param","text/plain");
            return;
        }
        std::filesystem::path fin_path = it;
        if(mapp.find(fin_path) != mapp.end()){
            auto name = mapp[fin_path].name;
            status_code = 200;  //this one is for OK.
            resp.status = status_code; 
            resp.set_content("the name of the file is:"+ string(name),"text/plain");
        }
        else{
            status_code = 404; //not found.
            resp.status = status_code;
            resp.set_content("The file with the path could not be found in the directoy :"+
                    (string)base_dir,"text/plain");
        }
    });
    svr.Get("/root_dir",[](const Request &req, Response &resp){
        resp.status = 200;
        resp.set_content("the base dir is:" + (string)base_dir,"text/plain");
    });
    svr.listen("0.0.0.0", 8080);  ////IMP -> The IP needs to be changed.
}