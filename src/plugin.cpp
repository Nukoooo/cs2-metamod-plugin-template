#include "plugin.hpp"
#include "modules.hpp"

DEFINE_LOGGING_CHANNEL_NO_TAGS(LOG_CS2S, "Plogon", 0, LV_MAX, Color());

Plugin g_plugin(LOG_CS2S);
PLUGIN_EXPOSE(Plugin, g_plugin);

Plugin::Plugin(const LoggingChannelID_t logging)
{
    _log = logging;
}

Plugin::~Plugin() = default;

bool Plugin::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
    PLUGIN_SAVEVARS()

    mods::Init();

    _metamod = ismm;
    
    ismm->AddListener(this, this);

    return true;
}

bool Plugin::Unload(char* error, size_t maxlen)
{
    return ISmmPlugin::Unload(error, maxlen);
}

void Plugin::FireGameEvent(IGameEvent* event)
{
}
