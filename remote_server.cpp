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
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "yaml-cpp/yaml.h"
#include <functional>

using namespace std;
using namespace httplib;
using namespace rapidjson;

//the idea is to run this code on a remoter server, and set up http server on the machine that runs this
//and parser_with_yaml. It will query incoming requests and also periodically post data to the central cache server.
//the metadata_common is storing the global map for the path, meta data.
//Set up http server that is going to going to service REST API based query requests.
void push_metadata(string &central_ip,int port){
    while(true){
        this_thread::sleep_for(chrono::minutes(25));
        std::ifstream ipstream("metadata.json");
        if(!ipstream) continue;
        stringstream buffer;
        buffer << ipstream.rdbuf();
        ipstream.close();
        //sending it to the central cache server.
        string POST_URL = "http://"+central_ip+":"+to_string(port);
        Client cli(central_ip,port);    
        auto res = cli.Post("/metadata_full",buffer.str(),"application/json");
        if(res && res->status == 200){
            cout << "[$INFO$]The metadata.json file was sent successfully to the central node" << endl;
        }
        else{
            cerr << "[$ERROR$]The metadata.json file could not be sent to central node" << endl;
        }
    }
}

void machine_ack(string &central_ip, YAML::Node &machine_config,int port){
    //sending the machine's ip, name and uuid to the central server during startup.
    Document doc;
    doc.SetObject();
    Document::AllocatorType &allocator = doc.GetAllocator();
    //allocator to allot memory to the entries in the json tree in the memory has been made.
    Value entry(kObjectType);
    string uuid;
    string machine_ip;
    string machine_no;
    uuid = machine_config["machine"]["id"].as<string>();
    machine_ip = machine_config["machine"]["ip"].as<string>();
    int port_no = machine_config["machine"]["port"].as<int>();
    machine_no = machine_config["machine"]["hostname"].as<string>();
    entry.AddMember("uuid",Value(uuid.c_str(),allocator),allocator);
    entry.AddMember("machine_ip",Value(machine_ip.c_str(),allocator),allocator);
    entry.AddMember("port_no",port_no,allocator);
    entry.AddMember("machine_no",Value(machine_no.c_str(),allocator),allocator);
    doc.AddMember("machine_info", entry, allocator);
    try{
        StringBuffer buf;
        Writer<StringBuffer> writer(buf);
        doc.Accept(writer);
        ofstream ofs("machine_info.json");
        if(!ofs){
            throw ios_base::failure("Failed to open the machine_info.json file");
        }
        ofs << buf.GetString();
        if(!ofs.good()){
            throw ios_base::failure("Error occured while writing to the metadata.json");
        }
        ofs.close();
    }
    catch(const ios_base::failure &e){
        cout << "File I/O error" << endl;
    }
    catch(const exception &e){
        cout << "Exception was found" << endl;
    }
    //the file has now been made(called machine_info.json).
    std::ifstream ipstream("machine_info.json");
    if(!ipstream) cout << "Could not open the machine_info.json file";
    stringstream buffer;
    buffer << ipstream.rdbuf();
    ipstream.close();
    Client cli(central_ip,port);    
    auto res = cli.Post("/machine_info",buffer.str(),"application/json");
    if(res && res->status == 200){
        cout << "[$INFO$]The machine_info.json file was sent successfully to the central node" << endl;
    }
    else{
        cerr << "[$ERROR$]The machine_info.json file could not be sent to central node" << endl;
    }
}

void run_server(){
    YAML::Node yml = YAML::LoadFile("config.yaml");
    string central_ip = yml["central_server"]["ip"].as<string>();
    int port = yml["central_server"]["port"].as<int>();
    machine_ack(central_ip,yml,port);
    std::thread t([&central_ip, port]() {
        push_metadata(central_ip, port);
    });
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
    svr.listen("0.0.0.0", 5000);  
}