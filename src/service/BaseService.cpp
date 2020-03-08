//
// Created by yengsu on 2020/2/23.
//

#include "BaseService.h"
#include "../utils/JsonSerializer.h"

#include <cppcms/http_response.h>
#include <cppcms/url_dispatcher.h>

BaseService::BaseService(cppcms::service &srv)
	: application(srv)
{
}

void BaseService::main(std::string str_url)
{
	m_str_action = str_url;
	response().content_type("application/json; charset=utf-8");
    if(!dispatcher().dispatch(str_url))
    {
        response().status(cppcms::http::response::not_found);
        response().out() << json_serializer(cppcms::http::response::not_found, str_url, "api not fount");
    }
}
