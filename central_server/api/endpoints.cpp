#include <iostream>
#include <string.h>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <ios>
#include <fstream>
#include <chrono>
#include <thread>
#include <sstream>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "yaml-cpp/yaml.h"
#include "rapidjson/prettywriter.h"
#include "httplib.h"
#include "common_data.h"
#include <mutex>
#include <pqxx/pqxx>

using namespace std;
using namespace std::filesystem;
using namespace httplib;
using namespace rapidjson;

struct FileMetadata {
    std::string id;
    std::string file_path;
    std::string file_name;
    int64_t size_bytes;
    std::string modified_time;
};

void search_db(const string &name_filter){
    try{
        pqxx::connection c("host=localhost port=5432 dbname=postgres user=postgres password=kavurdo1");
        pqxx::work txn(c);
        string qry = "SELECT uuid, file_path, file_name, size_bytes, modified_time FROM file_metadata WHERE file_name ILIKE " +
            txn.quote("%" + name_filter + "%");
        pqxx::result R = txn.exec(qry);
        if(R.empty()){
            cout << "No files with the name " << name_filter << " were retrieved" << endl;
        }
        else{
            for(const auto &row : R){
                cout << "[DB] UUID :"<< row["uuid"].c_str()
                << " | Path: " << row["file_path"].c_str()
                << " | Name: " << row["file_name"].c_str()
                << " | Size: " << row["size_bytes"].as<int64_t>()
                << " | Modified: " << row["modified_time"].c_str() << "\n";
            }
        }
        txn.commit();
        c.disconnect();
    }
    catch(const exception &e){
        cerr << "DB error" << e.what() << "\n";
    }
}

void run_user_cli(){
    while(true){
        cout << "\nEnter a query:\n";
        cout << "--search <filename> OR\n";
        cout << "--uuid <uuid> --path <path_to_file> --attr <size|time|name|root|exit>\n> ";
        string cmd;
        cin >> cmd;
        if (cmd == "--search") {
            string filename_filter;
            cin >> filename_filter;
            search_db(filename_filter);
        }
        else if (cmd == "--uuid") {
            string uuid, path, attr;
            cin >> uuid >> cmd;
            if (cmd != "--path") {
                cout << "Invalid command.\n";
                continue;
            }
            cin >> path >> cmd;
            if (cmd != "--attr") {
                cout << "Invalid command.\n";
                continue;
            }
            cin >> attr;
            if (attr == "exit") break;
            if(uuid_mac.find(uuid) == uuid_mac.end()){
                cout << "[Error] UUID not found.\n";
                continue;
            }
            string remote_ip = uuid_mac[uuid].machine_ip;
            int remote_port = uuid_mac[uuid].machine_port;
            Client cli(remote_ip, remote_port);
            string endpoint;
            if (attr == "size") endpoint = "/file_size";
            else if (attr == "time") endpoint = "/last_time";
            else if (attr == "name") endpoint = "/name";
            else if (attr == "root") endpoint = "/root_dir";
            else {
                cout << "[Error] Unknown attribute requested.\n";
                continue;
            }
            auto res = cli.Get((endpoint + "?path=" + path).c_str());
            if (res && res->status == 200) {
                cout << "[Response] " << res->body << "\n";
            } else {
                cout << "[Error] Failed to get response. Status: ";
                cout << (res ? to_string(res->status) : "Connection error") << "\n";
            }
        }
        else {
            cout << "Invalid command.\n";
        }
    }
}

void incoming_machine_data(Server &central_svr){

    central_svr.Post("/machine_info",[](const Request &req, Response &rst){
        try{
            Document doc;
            if (doc.Parse(req.body.c_str()).HasParseError()) {
                throw runtime_error("Could not parse the machine info json data");
            }
            if (!doc.IsObject()) {
                throw runtime_error("JSON is not an object");
            }
            if (!doc.HasMember("machine_info") || !doc["machine_info"].IsObject()) {
                throw runtime_error("Missing or invalid 'machine_info' object");
            }
            const Value& info = doc["machine_info"];
            vector<string> required_keys = { "machine_ip", "uuid", "port_no", "machine_no" };
            for (const auto& key : required_keys) {
                if (!info.HasMember(key.c_str())) {
                    throw runtime_error("Missing required field: " + key);
                }
            }
            if (!info["machine_ip"].IsString() || !info["uuid"].IsString() || 
                !info["port_no"].IsInt() || !info["machine_no"].IsString()) {
                throw runtime_error("Incorrect types in JSON fields");
            }
            string machine_ip = info["machine_ip"].GetString();
            string uuid = info["uuid"].GetString();
            int remote_port = info["port_no"].GetInt();
            string machine_no = info["machine_no"].GetString();
            uuid_machine temp(machine_no,machine_ip,remote_port);
            {
                std::lock_guard<mutex> lock(central_mutex); 
                if(uuid_mac.find(uuid) != uuid_mac.end()){
                    cout << "Already received the info for this machine with uuid:" << uuid << endl;
                    return;
                }else{
                    uuid_mac[uuid] = temp;
                }
                int ulim = 4000;
                for(int i =1; i <= ulim; i++){
                    if(machines_found.find(i) == machines_found.end()){
                        machines_found.insert(i);
                        no_uuid[i] = uuid;
                        break;
                    }else{
                        continue;
                    }
                }
            }
            rst.set_content("The machine info was received","text/plain");
            rst.status = 200;
        }
        catch(const exception &e){
            cerr << "Error occured while trying to read from the machine info" << e.what() << endl;
            rst.set_content("The info could not be read due to error","text/plain");
            rst.status = 400;
        }
    });
}

void incoming_api(Server &central_svr){
    cout << "hi" << "\n";
    //incoming data from the remote servers is going to be stored in here.
    static mutex file_mutex;
    central_svr.Post("/metadata_full",[](const Request &req, Response &resp){
        try{
            Document doc; 
            //cout << "[DEBUG] Received body: " << req.body << endl;
            if (doc.Parse(req.body.c_str()).HasParseError()) {
                throw runtime_error("JSON parse error");
            }
            if( !doc.HasMember("uuid") || !doc.HasMember("metadata")){
                throw runtime_error("Missing required fields");
            }
            StringBuffer buf;
            PrettyWriter<StringBuffer> writer(buf);
            doc.Accept(writer);
            string uuid = doc["uuid"].GetString();
            const Value& metadata = doc["metadata"];
            if (!metadata.IsObject()) {
                throw runtime_error("Metadata is not an object");
            }
            pqxx::connection c("host=localhost port=5432 dbname=postgres user=postgres password=kavurdo1");
            pqxx::work txn(c);
            //-----------------------------SEND TO DATABASE/CACHE HERE------------------------------------//
            for (auto itr = metadata.MemberBegin(); itr != metadata.MemberEnd(); itr++) {
                string file_path = itr->name.GetString();
                const Value& entry = itr->value;
                if (!entry.IsObject()) continue;
                string file_name = entry["Filename"].GetString();
                int64_t size_bytes = entry["Size"].GetInt64();
                string modified_time = entry["modified_time"].GetString();
                pqxx::result r = txn.exec_params("SELECT 1 FROM file_metadata WHERE uuid=$1 AND file_path=$2", uuid, file_path);
                if(r.empty()){
                    txn.exec_params(
                        "INSERT INTO file_metadata(uuid,file_path,file_name,size_bytes,modified_time) "
                        "VALUES ($1,$2,$3,$4,$5);",
                        uuid,
                        file_path,
                        file_name,
                        size_bytes,
                        modified_time
                    );
                }
            }
            txn.commit();
            cout << "data was inserted into the database successfully" << endl;
            resp.set_content("The metadata was successfully stored in the database", "text/plain");
            resp.status = 200;
        }
        catch (const exception &e) {
            cerr << "[$ERROR$] Error while processing metadata: " << e.what() << endl;
            resp.set_content("Failed to store metadata", "text/plain");
            resp.status = 400;
        }
    });
    central_svr.Get("/search", [](const Request &req, Response &res) {
        if (!req.has_param("name")) {
            res.set_content("Missing name parameter", "text/plain");
            res.status = 400;
            return;
        }
        string name_filter = req.get_param_value("name");
        try {
            pqxx::connection c("host=localhost port=5432 dbname=postgres user=postgres password=kavurdo1");
            pqxx::work txn(c);
            string qry = "SELECT uuid, file_path, file_name, size_bytes, modified_time "
                        "FROM file_metadata WHERE file_name ILIKE " + txn.quote("%" + name_filter + "%");
            pqxx::result R = txn.exec(qry);
            StringBuffer buffer;
            Writer<StringBuffer> writer(buffer);
            writer.StartArray();
            for (const auto& row : R) {
                writer.StartObject();
                writer.Key("uuid"); writer.String(row["uuid"].c_str());
                writer.Key("file_path"); writer.String(row["file_path"].c_str());
                writer.Key("file_name"); writer.String(row["file_name"].c_str());
                writer.Key("size_bytes"); writer.Int64(row["size_bytes"].as<int64_t>());
                writer.Key("modified_time"); writer.String(row["modified_time"].c_str());
                writer.EndObject();
            }
            writer.EndArray();
            res.set_content(buffer.GetString(), "application/json");
            res.status = 200;
        } catch (const exception &e) {
            res.set_content("Error during DB search", "text/plain");
            res.status = 500;
        }
    });
}


