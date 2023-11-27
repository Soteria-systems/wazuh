/*
 * Wazuh - Indexer connector.
 * Copyright (C) 2015, Wazuh Inc.
 * June 2, 2023.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include "indexerConnector.hpp"
#include "HTTPRequest.hpp"
#include "loggerHelper.h"
#include "secureCommunication.hpp"
#include "serverSelector.hpp"
#include <fstream>

#define IC_NAME "indexer-connnector"

// TODO: remove the LCOV flags when the implementation of this class is completed
// LCOV_EXCL_START
std::unordered_map<IndexerConnector*, std::unique_ptr<ThreadDispatchQueue>> QUEUE_MAP;

// Single thread because the events needs to be processed in order.
constexpr auto DATABASE_WORKERS = 1;
constexpr auto DATABASE_BASE_PATH = "queue/indexer/";

IndexerConnector::IndexerConnector(
    const nlohmann::json& config,
    const std::string& templatePath,
    const std::function<
        void(const int, const std::string&, const std::string&, const int, const std::string&, const std::string&)>&
        logFunction)
{
    if (logFunction)
    {
        Log::assignLogFunction(logFunction);
    }
    // Initialize publisher.
    auto selector = std::make_shared<ServerSelector>(config.at("hosts"));

    // Get index name.
    auto indexName {config.at("name").get_ref<const std::string&>()};

    std::string caRootCertificate;
    std::string sslCertificate;
    std::string sslKey;
    std::string username;
    std::string password;

    auto secureCommunication = SecureCommunication::builder();

    if (config.contains("ssl"))
    {
        if (config.at("ssl").contains("certificate_authorities") &&
            !config.at("ssl").at("certificate_authorities").empty())
        {
            caRootCertificate = config.at("ssl").at("certificate_authorities").front().get_ref<const std::string&>();
        }

        if (config.at("ssl").contains("certificate"))
        {
            sslCertificate = config.at("ssl").at("certificate").get_ref<const std::string&>();
        }

        if (config.at("ssl").contains("key"))
        {
            sslKey = config.at("ssl").at("key").get_ref<const std::string&>();
        }
    }

    if (config.contains("username") && config.contains("password"))
    {
        username = config.at("username").get_ref<const std::string&>();
        password = config.at("password").get_ref<const std::string&>();
    }

    secureCommunication.basicAuth(username + ":" + password)
        .sslCertificate(sslCertificate)
        .sslKey(sslKey)
        .caRootCertificate(caRootCertificate);

    // Read template file.
    std::ifstream templateFile(templatePath);
    if (!templateFile.is_open())
    {
        throw std::runtime_error("Could not open template file.");
    }
    nlohmann::json templateData = nlohmann::json::parse(templateFile);

    // Try to initialize data in the wazuh-indexer.
    try
    {
        initialize(templateData, indexName, selector, secureCommunication);
    }
    catch (const std::exception& e)
    {
        logWarn(IC_NAME,
                "Error initializing IndexerConnector: %s"
                ", we will try again later.",
                e.what());
    }

    QUEUE_MAP[this] = std::make_unique<ThreadDispatchQueue>(
        [=](std::queue<std::string>& dataQueue)
        {
            try
            {
                if (!m_initialized)
                {
                    initialize(templateData, indexName, selector, secureCommunication);
                }

                auto url = selector->getNext();
                std::string bulkData;
                url.append("/_bulk");

                while (!dataQueue.empty())
                {
                    auto data = dataQueue.front();
                    dataQueue.pop();
                    auto parsedData = nlohmann::json::parse(data);
                    auto id = parsedData.at("id").get_ref<const std::string&>();

                    if (parsedData.at("operation").get_ref<const std::string&>().compare("DELETED") == 0)
                    {
                        bulkData.append(R"({"delete":{"_index":")");
                        bulkData.append(indexName);
                        bulkData.append(R"(","_id":")");
                        bulkData.append(id);
                        bulkData.append(R"("}})");
                        bulkData.append("\n");
                    }
                    else
                    {
                        bulkData.append(R"({"index":{"_index":")");
                        bulkData.append(indexName);
                        bulkData.append(R"(","_id":")");
                        bulkData.append(id);
                        bulkData.append(R"("}})");
                        bulkData.append("\n");
                        bulkData.append(parsedData.at("data").dump());
                        bulkData.append("\n");
                    }
                }
                // Process data.
                HTTPRequest::instance().post(
                    HttpURL(url),
                    bulkData,
                    [&](const std::string& response) { logDebug2(IC_NAME, "Response: %s", response.c_str()); },
                    [&](const std::string& error, const long statusCode)
                    {
                        // TODO: Need to handle the case when the index is not created yet, to avoid losing data.
                        logError(IC_NAME, "Error: %s, status code: %ld", error.c_str(), statusCode);
                    },
                    "",
                    DEFAULT_HEADERS,
                    secureCommunication);
            }
            catch (const std::exception& e)
            {
                logError(IC_NAME, "Error: %s", e.what());
            }
        },
        DATABASE_BASE_PATH + indexName,
        DATABASE_WORKERS);
}

IndexerConnector::~IndexerConnector()
{
    QUEUE_MAP.erase(this);
}

void IndexerConnector::publish(const std::string& message)
{
    QUEUE_MAP[this]->push(message);
}

void IndexerConnector::initialize(const nlohmann::json& templateData,
                                  const std::string& indexName,
                                  const std::shared_ptr<ServerSelector>& selector,
                                  const SecureCommunication& secureCommunication)
{
    // Initialize template.
    HTTPRequest::instance().put(
        HttpURL(selector->getNext() + "/_index_template/" + indexName + "_template"),
        templateData,
        [&](const std::string& response) {},
        [&](const std::string& error, const long) { throw std::runtime_error(error); },
        "",
        DEFAULT_HEADERS,
        secureCommunication);

    // Initialize Index.
    HTTPRequest::instance().put(
        HttpURL(selector->getNext() + "/" + indexName),
        templateData.at("template"),
        [&](const std::string& response) {},
        [&](const std::string& error, const long statusCode)
        {
            if (statusCode != 400)
            {
                throw std::runtime_error(error);
            }
        },
        "",
        DEFAULT_HEADERS,
        secureCommunication);

    m_initialized = true;
}
// LCOV_EXCL_STOP