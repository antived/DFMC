#ifndef METADATA_COMMON_H
#define METADATA_COMMON_H

#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>


class meta_data{
    public:
        std::filesystem::path name;
        uint32_t Size;
        std::filesystem::file_time_type last_time;
};

extern std::unordered_map<std::filesystem::path, meta_data>mapp;
extern std::filesystem::path base_dir;
void parser();
void run_server(const std::string& config_path);
void push_metadata();

#endif