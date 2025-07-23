# Distributed File Metadata Indexer

A lightweight distributed system to index and query file metadata across multiple machines. Each client machine parses its file system, extracts metadata, and sends it to a central C++ server that stores it in a PostgreSQL database. The server supports querying based on file names, sizes, and timestamps.

---

 Features

- Recursively scans directories and collects:
  - Filename
  - Size (in bytes)
  - Last modified time
- Sends metadata as JSON to a central server via HTTP POST
- Central server parses JSON using [RapidJSON](https://github.com/Tencent/rapidjson) and stores in PostgreSQL
- CLI interface to query files by name, size, or modification time
- Modular design — easily extendable for new attributes or metadata types

---


## Requirements

## Client-Side
C++17 or later
RapidJSON
Filesystem library (C++17 <filesystem>)

## Server-Side
C++17
libpqxx (PostgreSQL C++ connector)
cpp-httplib
PostgreSQL 13 or later

## DATABASE SETUP 

psql -U postgres -d postgres -W

CREATE TABLE file_metadata (
    id SERIAL PRIMARY KEY,
    uuid TEXT,
    file_path TEXT,
    file_name TEXT,
    size_bytes BIGINT,
    modified_time TEXT
);

CREATE TABLE machine_ip (
    uuid TEXT,
    ip TEXT
);

## SAMPLE QUERY
--search <filename>

--uuid <uuid> --path <file_path> --attr <name|size|time|root>

