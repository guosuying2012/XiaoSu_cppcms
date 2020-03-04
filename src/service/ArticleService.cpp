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

ArticleService::ArticleService(cppcms::service &srv)
	: BaseService(srv)
{
	dispatcher().map("GET", "/all_articles/page/(\\d+)/(\\d+)", &ArticleService::allArticles, this, 1, 2);
	dispatcher().map("GET", "/all_articles", &ArticleService::allArticles, this);

	dispatcher().map("GET", "/all_article_by_category/page/(.*)/(\\d+)/(\\d+)", &ArticleService::allArticleByCategory, this, 1, 2, 3);
	dispatcher().map("GET", "/all_article_by_category/(.*)", &ArticleService::allArticleByCategory, this, 1);

	dispatcher().map("GET", "/info/(.*)", &ArticleService::articleInfo, this, 1);
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
		//PLOG_ERROR << ex.what();
		response().out() << json_serializer(500, action(), ex.what());
	}

}

void ArticleService::articleInfo(const std::string& strArticleId)
{
	cpr::Url url;
	cpr::Session session;
	cpr::Response res;

	url = cpr::Url("https://wqxmwj4a.lc-cn-n1-shared.com/1.1/classes/Comment");

	auto header = cpr::Header {
			{ "X-LC-Id", "wqxMwj4aLtSLQEFVFUppwfqE-gzGzoHsz" },
			{ "X-LC-Key", "ra4Tmr8qsqbDNPTANTHBd1YF" },
			{ "Content-Type", "application/json" }
	};

	auto commentCount = cpr::Parameters {
			{"where", R"({"url":"2939d7c885114c8efaf5b0b384cbe8fd"})"},
			{"count", "1"},
			{"limit", "0"}
	};

	//view count
	auto viewCount = cpr::Parameters {
			{"where", R"({"url":"fde108310a67a89dd43c461ba9e79fce"})"},
			{"keys", "time"}
	};

	session.SetHeader(header);
	session.SetVerifySsl(true);

	session.SetUrl(url);
	session.SetParameters(commentCount);
	res = session.Get();

	if (res.error.code == cpr::ErrorCode::OK)
	{
		response().out() << res.text;
		return;
	}

	PLOG_ERROR << res.error.message;
	response().out() << json_serializer((int)res.error.code, action(), res.error.message);
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
		//找到所有文章
		Articles vecArticles = pSession->find<Article>();
		//计算可以分页的页数
		int nPages = nPageSize / vecArticles.size();
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
	try
	{
		const std::unique_ptr<dbo::Session>& pSession = dbo_session();
		dbo::Transaction transaction(*pSession);

		dbo::ptr<Category> pCategory = pSession->find<Category>().where("category_id=?").bind(strCategoryId);

		if (pCategory.get() == nullptr)
		{
			response().out() << json_serializer(301, action(), "此分类下没有内容");
			return;
		}

		response().out() << json_serializer(pCategory->getArticles(), 200, action(), "success!");
	}
	catch (const std::exception& ex)
	{
		PLOG_ERROR << ex.what();
		response().out() << json_serializer(500, action(), ex.what());
	}
}

void ArticleService::allArticleByCategory(const std::string& strCategoryId, int nPageSize, int nCurrentPage)
{
	std::stringstream ss;
	ss << "allArticleByCategory2: " << strCategoryId << ", size: " << nPageSize << ", current: " << nCurrentPage;

	response().out() << json_serializer(200, action(), ss.str());
}