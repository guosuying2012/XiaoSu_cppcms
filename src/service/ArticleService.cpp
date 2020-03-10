//
// Created by yengsu on 2020/3/1.
//

#include "../define.h"
#include "../model/user.h"
#include "../model/userinfo.h"
#include "../model/category.h"
#include "../model/article.h"
#include "../utils/JsonSerializer.h"
#include "ArticleService.h"

#include <cppcms/http_response.h>
#include <cppcms/url_dispatcher.h>
#include <exception>

//test
#include "../utils/JsonUnserializer.h"
#include <cppcms/http_request.h>

ArticleService::ArticleService(cppcms::service &srv)
	: BaseService(srv)
{
	//获取文章列表
	dispatcher().map("GET", "/all_articles/page/(\\d+)/(\\d+)", &ArticleService::allArticles, this, 1, 2);
	dispatcher().map("GET", "/all_articles", &ArticleService::allArticles, this);

	//根据分类获取文章列表
	dispatcher().map("GET", "/all_article_by_category/page/(.*)/(\\d+)/(\\d+)", &ArticleService::allArticleByCategory, this, 1, 2, 3);
	//test
	dispatcher().map("POST", "/all_article_by_category/(.*)", &ArticleService::allArticleByCategory, this, 1);

	//根据用户获取文章列表
	dispatcher().map("GET", "/all_article_by_user/page/(.*)/(\\d+)/(\\d+)", &ArticleService::allArticleByUser, this, 1, 2, 3);
	dispatcher().map("GET", "/all_article_by_user/(.*)", &ArticleService::allArticleByUser, this, 1);

	//文章详细内容
	//dispatcher().map("GET", "/info/(.*)", &ArticleService::articleInfo, this, 1);
	dispatcher().map("GET", "/(.*)", &ArticleService::article, this, 1);
}

void ArticleService::article(const std::string& strArticleId)
{
	try
	{
		const std::unique_ptr<dbo::Session>& pSession = dbo_session();
		dbo::Transaction transaction(*pSession);

		dbo::ptr<Article> pArticle = pSession->find<Article>().where("article_id=?").bind(strArticleId);

		if (pArticle.get() == nullptr)
		{
			response().out() << json_serializer(301, action(), "article not found");
			return;
		}

		response().out() << json_serializer(pArticle, 200, action(), "success");
	}
	catch (const std::exception& ex)
	{
		PLOG_ERROR << ex.what();
		response().out() << json_serializer(500, action(), ex.what());
	}

}

void ArticleService::allArticles()
{
	try
	{
		const std::unique_ptr<dbo::Session>& pSession = dbo_session();
		dbo::Transaction transaction(*pSession);
		Articles vecArticles = pSession->find<Article>();
		response().out() << json_serializer(vecArticles, 200, action(), "success!");
	}
	catch (const std::exception& ex)
	{
		PLOG_ERROR << ex.what();
		response().out() << json_serializer(500, action(), ex.what());
	}
}

void ArticleService::allArticles(int nPageSize, int nCurrentPage)
{
	try
	{
		const std::unique_ptr<dbo::Session>& pSession = dbo_session();
		dbo::Transaction transaction(*pSession);
		Articles vecArticles = pSession->find<Article>();
		vecArticles = pSession->find<Article>()
		        .offset((nCurrentPage -1 ) * nPageSize)
		        .limit(nPageSize);

		response().out() << json_serializer(vecArticles, 200, action(), "success!");
	}
	catch (const std::exception& ex)
	{
		PLOG_ERROR << ex.what();
		response().out() << json_serializer(500, action(), ex.what());
	}
}

void ArticleService::allArticleByUser(const std::string &strUserId)
{
	try
	{
		const std::unique_ptr<dbo::Session>& pSession = dbo_session();
		dbo::Transaction transaction(*pSession);

		dbo::ptr<User> pUser = pSession->find<User>().where("user_id=?").bind(strUserId);

		if (pUser.get() == nullptr)
		{
			response().out() << json_serializer(301, action(), "指定用户不存在");
			return;
		}

		response().out() << json_serializer(pUser->getArticles(), 200, action(), "success!");
	}
	catch (const std::exception& ex)
	{
		PLOG_ERROR << ex.what();
		response().out() << json_serializer(500, action(), ex.what());
	}
}

void ArticleService::allArticleByUser(const std::string &strUserId, int nPageSize, int nCurrentPage)
{
	try
	{
		const std::unique_ptr<dbo::Session>& pSession = dbo_session();
		dbo::Transaction transaction(*pSession);
		Articles vecArticles = pSession->find<Article>();
		vecArticles = pSession->find<Article>()
				.where("user_id=?")
				.bind(strUserId)
				.offset((nCurrentPage -1 ) * nPageSize)
				.limit(nPageSize);

		response().out() << json_serializer(vecArticles, 200, action(), "success!");
	}
	catch (const std::exception& ex)
	{
		PLOG_ERROR << ex.what();
		response().out() << json_serializer(500, action(), ex.what());
	}
}

void ArticleService::allArticleByCategory(const std::string& strCategoryId)
{
    dbo::ptr<Category> pCategory;
	JsonUnserializer jsonUnserializer;

	std::string raw = (char*)request().raw_post_data().first;
    jsonUnserializer.unserialize(raw, pCategory);
    PLOG_INFO << raw;

//    const std::unique_ptr<dbo::Session>& pSession = dbo_session();
//    dbo::Transaction transaction(*pSession);
//    dbo::ptr<Category> pUser = pSession->find<Category>()
//            .where("category_id=?")
//            .bind("28");
    response().out() << json_serializer(pCategory, 200, "", "");

	/*try
	{
		const std::unique_ptr<dbo::Session>& pSession = dbo_session();
		dbo::Transaction transaction(*pSession);

		dbo::ptr<Category> pCategory = pSession->find<Category>().where("category_id=?").bind(strCategoryId);

		if (pCategory.get() == nullptr)
		{
			response().out() << json_serializer(301, action(), "没有找到相关分类");
			return;
		}

		response().out() << json_serializer(pCategory->getArticles(), 200, action(), "success!");
	}
	catch (const std::exception& ex)
	{
		PLOG_ERROR << ex.what();
		response().out() << json_serializer(500, action(), ex.what());
	}*/
}

void ArticleService::allArticleByCategory(const std::string& strCategoryId, int nPageSize, int nCurrentPage)
{
	try
	{
		const std::unique_ptr<dbo::Session>& pSession = dbo_session();
		dbo::Transaction transaction(*pSession);
		Articles vecArticles = pSession->find<Article>();
		vecArticles = pSession->find<Article>()
		        .where("category_id=?")
		        .bind(strCategoryId)
				.offset((nCurrentPage -1 ) * nPageSize)
				.limit(nPageSize);

		response().out() << json_serializer(vecArticles, 200, action(), "success!");
	}
	catch (const std::exception& ex)
	{
		PLOG_ERROR << ex.what();
		response().out() << json_serializer(500, action(), ex.what());
	}
}
