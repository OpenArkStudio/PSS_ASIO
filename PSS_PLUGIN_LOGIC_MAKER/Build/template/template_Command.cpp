#include "[command head file]"

void [class name]::Init(ISessionService* session_service)
{
    session_service_ = session_service;
	
	[do message logic init]

    PSS_LOGGER_DEBUG("[[class name]::Init]init.");
}

[command logic function achieve]