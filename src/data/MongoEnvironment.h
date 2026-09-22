#pragma once

#include <mongocxx/instance.hpp>

class MongoEnvironment
{
public:
    MongoEnvironment();

private:
    mongocxx::instance m_Instance;
};