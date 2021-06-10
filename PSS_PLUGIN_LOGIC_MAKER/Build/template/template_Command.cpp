#include "[command head file]"

void CBaseCommand::Init(ISessionService* session_service)
{
    session_service_ = session_service;

    PSS_LOGGER_DEBUG("[CBaseCommand::Init]init.");
}

[command logic function achieve]