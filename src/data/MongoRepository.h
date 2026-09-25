// Implements the MongoRepository class that interacts with MongoDB for data storage
#pragma once

#include <string>
#include <vector>

#include <bsoncxx/document/value.hpp>
#include <mongocxx/client.hpp>

class MongoRepository
{
public:
    MongoRepository(const std::string& connectionUri, std::string database);

    bool FindMany(const std::string& collection, std::vector<bsoncxx::document::value>& outDocuments);
    bool InsertMany(const std::string& collection, const std::vector<bsoncxx::document::value>& documents);

private:
    mongocxx::client m_Client;
    std::string m_Database;
};