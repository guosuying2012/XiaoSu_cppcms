//
// Created by yengsu on 2020/1/23.
//

#include "../model/SystemLog.h"
#include "../utils/JsonSerializer.h"
#include "ApplicationService.h"

#include <cppcms/http_response.h>
#include <cppcms/url_dispatcher.h>

ApplicationService::ApplicationService(cppcms::service& srv)
    :BaseService(srv)
{
	try
	{
		const std::unique_ptr<dbo::Session>& pSession = DboSingleton::GetInstance().GetSession();
		pSession->mapClass<SystemLog>("sys_runtime_log");
		pSession->createTables();
	}
	catch (const std::exception& e)
	{
	}

	dispatcher().map("GET", "/api", &ApplicationService::index, this);
}

