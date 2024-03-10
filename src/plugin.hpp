#pragma once

#include <ISmmPlugin.h>

#include <tier0/logging.h>
#include <igameeventsystem.h>  // Required by igameevents.h
#include <igameevents.h>

class Plugin : public ISmmPlugin, public IMetamodListener, public IGameEventListener2
{
public:
    explicit Plugin(LoggingChannelID_t logging);
    ~Plugin();
    // no move or copy
    Plugin(Plugin&&)      = delete;
    Plugin(const Plugin&) = delete;

    bool Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late) override;
    bool Unload(char* error, size_t maxlen) override;

    const char* GetAuthor() override
    {
        return "Nuko";
    }

    const char* GetName() override
    {
        return "Plogon";
    }

    const char* GetDescription() override
    {
        return "Plogon";
    }

    const char* GetURL() override
    {
        return "https://github.com/Nukoooo";
    }

    const char* GetLicense() override
    {
        return "MIT";
    }

    const char* GetVersion() override
    {
        return "0.1";
    }

    const char* GetDate() override
    {
        return "1970-01-01";
    }

    const char* GetLogTag() override
    {
        return "plogon";
    }

    void FireGameEvent(IGameEvent* event) override;

private:
    ISmmAPI* _metamod{};
    LoggingChannelID_t _log;
};
