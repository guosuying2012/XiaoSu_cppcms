//
// Created by yengsu on 2020/1/23.
//

#include "../model/SystemLog.h"
#include "../utils/JsonSerializer.h"
#include "ApplicationService.h"

#include <cppcms/url_mapper.h>
#include <cppcms/http_response.h>
#include <cppcms/url_dispatcher.h>

ApplicationService::ApplicationService(cppcms::service& srv)
    :cppcms::application(srv)
{
	try
	{
		const std::unique_ptr<dbo::Session>& pSession = DboSingleton::GetInstance().GetSession();
		pSession->mapClass<SystemLog>("sys_runtime_log");
		pSession->createTables();
	}
	catch (const std::exception& e)
	{
		//PLOG_ERROR << "DBO Error: " << e.what();
	}
}

void ApplicationService::main(std::string url)
{
	if(!dispatcher().dispatch(url))
	{
		response().status(cppcms::http::response::not_found);
		response().out() << json_serializer(404, url, "api not fount");
	}
}

