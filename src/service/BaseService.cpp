//
// Created by yengsu on 2020/2/23.
//

#include "BaseService.h"
#include "utils/JsonSerializer.h"

#include <cppcms/http_response.h>
#include <cppcms/url_dispatcher.h>
#include <cppcms/http_context.h>

BaseService::BaseService(cppcms::service &srv)
	: application(srv)
{
}

void BaseService::main(std::string strUrl)
{
	if (!strUrl.empty())
	{
		m_strAction = strUrl;
	}

    response().content_type("application/json; charset=utf-8");
    if(!dispatcher().dispatch(strUrl))
    {
        response().status(cppcms::http::response::not_found);
        response().out() << json_serializer(cppcms::http::response::not_found, strUrl, "未找到相关API");
    }
}
