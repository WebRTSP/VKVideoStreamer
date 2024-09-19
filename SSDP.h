#pragma once

#include <optional>
#include <string>
#include <deque>

#include <libgssdp/gssdp.h>


struct SSDPClient
{
    SSDPClient(SSDPClient&) = delete;
    SSDPClient& operator = (SSDPClient&) = delete;

    SSDPClient(GSSDPClient* client, GSSDPResourceGroup* resourceGroup) :
        client(client), resourceGroup(resourceGroup) {}
    ~SSDPClient() {
        g_object_unref(resourceGroup);
        g_object_unref(client);
    }

    GSSDPClient *const client;
    GSSDPResourceGroup *const resourceGroup;
};

struct SSDPContext {
    std::optional<std::string> deviceUuid;
    std::deque<SSDPClient> clients;
};

void SSDPPublish(SSDPContext* context);
