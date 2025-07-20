#include <iostream>
#include <string.h>
#include <map>
#include <vector>
#include <filesystem>
#include<cstdlib>
#include <ios>
#include <fstream>
#include <unordered_map>
#include <cstring>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <queue>        
#include <chrono>
#include "yaml-cpp/yaml.h"
#include "spdlog/spdlog.h"  
#include "spdlog/sinks/basic_file_sink.h"
#include <stdexcept>
#include "metadata_common.h"

using namespace rapidjson;
using namespace std;
using namespace std::filesystem;
/* This is to basically go through all of the files in the destop directory of the
    PC and then make a metadata json of all the metadata that is collected.*/

// void metadata_parser(std::filesystem::path){ //commmon datastruct to store the format of the metadata.

// namespace std{
//     template<>
//     struct hash<path>{
//         size_t operator()(const path &p) const{
//             return hash<string>()(p.string());
//         }
//     };
// }

std::unordered_map<std::filesystem::path, meta_data>mapp;
std::filesystem::path base_dir;

void parser(){
    if(!exists("logs")){
        create_directory("logs");
    }
    auto logger = spdlog::basic_logger_mt("file_logger","logs/scan.log");
    YAML::Node config_read = YAML::LoadFile("config.yaml");
    if (!config_read["parser"]) {
        throw std::runtime_error("Missing 'parser' section in config.yaml");
    }
    YAML::Node parser_node = config_read["parser"];
    if (!parser_node["scan_root"] || !parser_node["output_file"]) {
        throw std::runtime_error("Missing required fields under 'parser'");
    }
    base_dir = parser_node["scan_root"].as<std::string>();
    string output_filename = parser_node["output_file"].as<std::string>();
    cout << "Parsed base_dir: " << base_dir << ", output_file: " << output_filename << endl;
    logger->info("Scan started for the base directory: {}",base_dir.string());
    if(is_directory(base_dir) && exists(base_dir)){
        //get the metadata of all the files inside.
        //the directory will contain sub directories.
        queue<path> q;
        q.push(base_dir);
        while(!q.empty()){
            path tmp = q.front();
            q.pop();
            for(const auto &entry : directory_iterator(tmp)){
                const auto &j = entry.path();
                if(is_directory(j)){    
                    q.push(j);
                }
                else{
                    try{
                        //write the json metadata parsing logic here.
                        auto last_write_time_tmp = last_write_time(j);
                        uint32_t file_Size = file_size(j);
                        //we need the name and the absolute paths also.
                        std::filesystem::path tmp_pth = relative(j,tmp);
                        logger->info("Scanned the file and prepped metadata of:{}",tmp_pth.string());
                        meta_data holder;
                        holder.name = tmp_pth;
                        holder.Size = file_Size;
                        holder.last_time = last_write_time_tmp;
                        mapp[j] = holder;
                    }
                    catch(const std::filesystem::filesystem_error &e){
                        logger->warn("Skipping the file at {} due to error",j.string());
                        cerr << "Warning: Skipped file due to error" << e.what()<< "\n";
                    }
                }
            }
        }
    }
    //now we have to convert the map to its equivalent json format and store.
    Document doc;
    doc.SetObject();
    Document::AllocatorType &allocator = doc.GetAllocator();
    for(auto &[path_var,value] : mapp){
        Value entry(kObjectType);
        entry.AddMember("Filename", Value(value.name.string().c_str(),allocator),allocator);
        entry.AddMember("Size",value.Size,allocator);
        //converting the file_system clock to the system_clock.
        auto fixed = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            value.last_time - file_time_type::clock::now() + std::chrono::system_clock::now());
        time_t new_time = std::chrono::system_clock::to_time_t(fixed);
        string time_fin = asctime(localtime(&new_time));
        time_fin.pop_back(); //removing the \0.
        entry.AddMember("last modified",Value(time_fin.c_str(),allocator),allocator);
        doc.AddMember(Value(path_var.string().c_str(), allocator), entry, allocator);
    }
    //json has been structured, now will write to file..
    try{
        StringBuffer buf;
        Writer<StringBuffer> writer(buf);
        doc.Accept(writer);
        ofstream ofs(output_filename);
        if(!ofs){
            throw ios_base::failure("Failed to open the metadata.json file");
        }
        ofs << buf.GetString();
        if(!ofs.good()){
            throw ios_base::failure("Error occured while writing to the metadata.json");
        }
        ofs.close();
        logger->info("The data has been written to metadata.json, for all files in directory :{}",base_dir.string());
    }
    catch(const ios_base::failure &e){
        logger->error("File I/O Error : {}",e.what());
        cout << "File I/O error" << endl;
    }
    catch(const exception &e){
        logger->error("Caught exception {}",e.what());
        cout << "Exception was found" << endl;
    }
    logger->flush();
}

