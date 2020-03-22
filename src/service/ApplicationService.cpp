//
// Created by yengsu on 2020/1/23.
//

#include "model/user.h"
#include "model/userinfo.h"
#include "model/category.h"
#include "model/article.h"
#include "model/systemlog.h"

#include "ArticleService.h"
#include "service/ApplicationService.h"
#include "utils/JsonSerializer.h"

#include <cppcms/http_response.h>
#include <cppcms/url_dispatcher.h>

ApplicationService::ApplicationService(cppcms::service& srv)
    :BaseService(srv)
{
	try
	{
		//attach service
		attach(new ArticleService(srv),"article",
				"/web/api/v1/article{1}","/web/api/v1/article((/?.*))", 1);

		//map class
	    const std::unique_ptr<dbo::Session>& pSession = dbo_session();
		pSession->mapClass<User>("user");
        pSession->mapClass<UserInfo>("user_info");
		pSession->mapClass<Article>("article");
        pSession->mapClass<Category>("category");
        pSession->mapClass<SystemLog>("sys_runtime_log");
		pSession->createTables();
	}
	catch (const std::exception& ex)
	{
        //PLOG_DEBUG << ex.what();
	}
}

