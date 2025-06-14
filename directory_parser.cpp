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

using namespace rapidjson;
using namespace std;
using namespace std::filesystem;
using namespace rapidjson;
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
struct meta_data{
    std::filesystem::path name;
    uint32_t Size;
    std::filesystem::file_time_type last_time;
};

int main(){
    unordered_map<path, meta_data> mapp;    
    path base_dir = "/home/vedant/cpp_dev";
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
                        meta_data holder;
                        holder.name = tmp_pth;
                        holder.Size = file_Size;
                        holder.last_time = last_write_time_tmp;
                        mapp[j] = holder;
                    }
                    catch(const std::filesystem::filesystem_error &e){
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
    //json has been structured, now will write to file...
    StringBuffer buf;
    Writer<StringBuffer> writer(buf);
    doc.Accept(writer);
    ofstream ofs("metadata.json");
    ofs << buf.GetString();
    ofs.close();
    cout << "the metadata of all the files in the Desktop directory have been written to json file.";
}
