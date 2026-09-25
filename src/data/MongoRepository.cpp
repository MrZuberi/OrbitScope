// Contains source code for the MongoRepository.cpp file
#include "data/MongoRepository.h"

#include <mongocxx/uri.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/exception.hpp>

MongoRepository::MongoRepository(const std::string& connectionUri, std::string database)
    : m_Client(mongocxx::uri(connectionUri))
    , m_Database(std::move(database))
{
}

bool MongoRepository::FindMany(const std::string& collection, std::vector<bsoncxx::document::value>& outDocuments)
{
    try
    {
        mongocxx::database database = m_Client[m_Database];
        mongocxx::collection targetCollection = database[collection];

        outDocuments.clear();

        auto cursor = targetCollection.find({});
        for (const bsoncxx::document::view& document : cursor)
        {
            outDocuments.push_back(bsoncxx::document::value(document));
        }

        return true;
    }
    catch (const mongocxx::exception&)
    {
        return false;
    }
}

bool MongoRepository::InsertMany(const std::string& collection, const std::vector<bsoncxx::document::value>& documents)
{
    if (documents.empty())
    {
        return true;
    }

    try
    {
        mongocxx::database database = m_Client[m_Database];
        mongocxx::collection targetCollection = database[collection];

        std::vector<bsoncxx::document::view> views;
        for (const bsoncxx::document::value& document : documents)
        {
            views.push_back(document.view());
        }

        targetCollection.insert_many(views);
        return true;
    }
    catch (const mongocxx::exception&)
    {
        return false;
    }
}