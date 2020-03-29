//
// Created by yengsu on 2020/3/1.
//

#include "define.h"
#include "model/user.h"
#include "model/article.h"
#include "model/userinfo.h"
#include "model/category.h"
#include "ArticleService.h"
#include "utils/JsonSerializer.h"
#include "utils/JsonUnserializer.h"

#include <cppcms/http_request.h>
#include <cppcms/http_response.h>
#include <cppcms/url_dispatcher.h>
#include <exception>

ArticleService::ArticleService(cppcms::service &srv)
	: BaseService(srv)
{
    //GET

	//获取文章列表
    dispatcher().map("GET", "/all_articles/page/(\\d+)/(\\d+)", &ArticleService::all_articles, this, 1, 2);
    dispatcher().map("GET", "/all_articles", &ArticleService::all_articles, this);

	//根据分类获取文章列表
    dispatcher().map("GET", "/all_article_by_category/page/(.*)/(\\d+)/(\\d+)", &ArticleService::all_article_by_category, this, 1, 2, 3);
    dispatcher().map("GET", "/all_article_by_category/(.*)", &ArticleService::all_article_by_category, this, 1);

	//根据用户获取文章列表
    dispatcher().map("GET", "/all_article_by_user/page/(.*)/(\\d+)/(\\d+)", &ArticleService::all_article_by_user, this, 1, 2, 3);
    dispatcher().map("GET", "/all_article_by_user/(.*)", &ArticleService::all_article_by_user, this, 1);

    //添加
    dispatcher().map("POST", "", &ArticleService::article, this, 1);

    //文章详细内容
    dispatcher().map("GET", "/(.*)", &ArticleService::article, this, 1);

    //修改
    dispatcher().map("PUT", "/(.*)", &ArticleService::article, this, 1);

    //删除
    dispatcher().map("DELETE", "/(.*)", &ArticleService::article, this, 1);

    //移动
    dispatcher().map("PATCH", "/(.*)/move_to/(user|category)/(.*)", &ArticleService::move_to, this, 1, 2, 3);
}

void ArticleService::article(const std::string& strArticleId)
{
    try
    {
        const std::unique_ptr<dbo::Session>& pSession = dbo_session();
        dbo::Transaction transaction(*pSession);

        dbo::ptr<Article> pArticle = pSession->find<Article>()
                .where("article_id=?")
                .bind(strArticleId);

        //获取博客
        if (request().request_method() == "GET")
        {
            if (!pArticle)
            {
                response().out() << json_serializer(cppcms::http::response::not_found, action(), "article not found");
                return;
            }

            response().out() << json_serializer(pArticle, cppcms::http::response::ok, action(), "success");
            return;
        }

        //添加
        if (request().request_method() == "POST")
        {
            add_article();
            return;
        }

        //删除
        if (request().request_method() == "DELETE")
        {
            delete_article(strArticleId);
            return;
        }

        //修改
        if (request().request_method() == "PUT")
        {
            if (request().raw_post_data().second <= 0)
            {
                response().out() << json_serializer(cppcms::http::response::precondition_failed, action(), "修改失败");
                return;
            }

            const std::string& strRaw = static_cast<char*>(request().raw_post_data().first);
            dbo::ptr<Article> pModify = dbo::make_ptr<Article>();
            json_unserializer(strRaw, pModify);
            if (pModify->user())//用户
            {
                pArticle.modify()->user(pModify->user());
            }
            if (pModify->category())//分类
            {
                pArticle.modify()->category(pModify->category());
            }
            if (!pModify->title().empty())//标题
            {
                pArticle.modify()->title(pModify->title());
            }
            if (!pModify->cover().empty())//封面
            {
                pArticle.modify()->cover(pModify->cover());
            }
            if (!pModify->content().empty())//正文
            {
                pArticle.modify()->content(pModify->content());
            }
            if (!pModify->describe().empty())//简介
            {
                pArticle.modify()->describe(pModify->describe());
            }

            response().out() << json_serializer(cppcms::http::response::ok, action(), translate("修改成功"));
        }
    }
    catch (const std::exception& ex)
    {
        PLOG_ERROR << ex.what();
        response().out() << json_serializer(cppcms::http::response::internal_server_error, action(), ex.what());
    }

}

void ArticleService::add_article()
{
    try
    {
        const std::unique_ptr<dbo::Session>& pSession = dbo_session();
        dbo::Transaction transaction(*pSession);

        dbo::ptr<Article> pArticle = dbo::make_ptr<Article>();

        if (request().raw_post_data().second <= 0)
        {
            response().out() << json_serializer(cppcms::http::response::precondition_failed, action(), translate("添加失败,未收到需要添加的内容。"));
            return;
        }

        const std::string& strRaw = static_cast<char*>(request().raw_post_data().first);
        json_unserializer(strRaw, pArticle);

        //判断有效性
        if (!pArticle->category())
        {
            response().out() << json_serializer(cppcms::http::response::not_found, action(), translate("添加失败,未找到分类。"));
            return;
        }
        if (!pArticle->user())
        {
            response().out() << json_serializer(cppcms::http::response::not_found, action(), translate("添加失败,未找到用户。"));
            return;
        }

        //判断添加是否成功
        dbo::ptr<Article> pAddedPtr = pSession->add<Article>(pArticle);
        if (pAddedPtr)
        {
            response().out() << json_serializer(pAddedPtr, cppcms::http::response::created, action(), translate("添加成功"));
        }
        else
        {
            response().out() << json_serializer(cppcms::http::response::precondition_failed, action(), translate("添加失败"));
        }
    }
    catch (const std::exception& ex)
    {
        PLOG_ERROR << ex.what();
        response().out() << json_serializer(cppcms::http::response::internal_server_error, action(), translate(ex.what()));
    }
}

void ArticleService::delete_article(const std::string& strId)
{
    try
    {
        const std::unique_ptr<dbo::Session>& pSession = dbo_session();
        dbo::Transaction transaction(*pSession);

        dbo::ptr<Article> pArticle = pSession->find<Article>().where("article_id=?").bind(strId);
        if (!pArticle)
        {
            //失败，未找到相关
            response().out() << json_serializer(cppcms::http::response::not_found, action(), translate("删除失败,未找到博文."));
            return;
        }

        pArticle.remove();
        response().out() << json_serializer(cppcms::http::response::ok, action(), translate("删除成功"));
    }
    catch (const std::exception& ex)
    {
        PLOG_ERROR << ex.what();
        response().out() << json_serializer(cppcms::http::response::internal_server_error, action(), translate(ex.what()));
    }
}

void ArticleService::move_to(const std::string strArticleId, const std::string& strMoveTo, const std::string strId)
{
    try
    {
        const std::unique_ptr<dbo::Session>& pSession = dbo_session();
        dbo::Transaction transaction(*pSession);

        dbo::ptr<Article> pArticle = pSession->find<Article>().where("article_id=?").bind(strArticleId);
        if (!pArticle)
        {
            //失败，未找到相关
            response().out() << json_serializer(cppcms::http::response::not_found, action(), translate("移动失败,未找到博文."));
            return;
        }

        if (strMoveTo == "user")
        {
            dbo::ptr<User> pUser = pSession->find<User>().where("user_id=?").bind(strId);
            if (!pUser)
            {
                //未找到相关
                response().out() << json_serializer(cppcms::http::response::not_found, action(), translate("移动失败,未找到用户"));
                return;
            }

            pArticle.modify()->user(pUser);
        }

        if (strMoveTo == "category")
        {
            dbo::ptr<Category> pCategory = pSession->find<Category>().where("category_id=?").bind(strId);
            if (!pCategory)
            {
                //未找到相关
                response().out() << json_serializer(cppcms::http::response::not_found, action(), translate("移动失败,未找到分类"));
                return;
            }

            pArticle.modify()->category(pCategory);
        }

        response().out() << json_serializer(cppcms::http::response::ok, action(), translate("移动成功"));
    }
    catch (const std::exception& ex)
    {
        PLOG_ERROR << ex.what();
        response().out() << json_serializer(cppcms::http::response::internal_server_error, action(), translate(ex.what()));
    }
}

void ArticleService::all_articles()
{
	try
	{
		const std::unique_ptr<dbo::Session>& pSession = dbo_session();
		dbo::Transaction transaction(*pSession);
		Articles vecArticles = pSession->find<Article>();
        response().out() << json_serializer(vecArticles, cppcms::http::response::ok, action(), translate("success!"));
	}
	catch (const std::exception& ex)
	{
		PLOG_ERROR << ex.what();
        response().out() << json_serializer(cppcms::http::response::internal_server_error, action(), translate(ex.what()));
	}
}

void ArticleService::all_articles(int nPageSize, int nCurrentPage)
{
	try
	{
		const std::unique_ptr<dbo::Session>& pSession = dbo_session();
		dbo::Transaction transaction(*pSession);
		Articles vecArticles = pSession->find<Article>();
		vecArticles = pSession->find<Article>()
		        .offset((nCurrentPage -1 ) * nPageSize)
		        .limit(nPageSize);

        response().out() << json_serializer(vecArticles, cppcms::http::response::ok, action(), translate("success!"));
	}
	catch (const std::exception& ex)
	{
		PLOG_ERROR << ex.what();
        response().out() << json_serializer(cppcms::http::response::internal_server_error, action(), translate(ex.what()));
	}
}

void ArticleService::all_article_by_user(const std::string &strUserId)
{
	try
	{
		const std::unique_ptr<dbo::Session>& pSession = dbo_session();
		dbo::Transaction transaction(*pSession);

		dbo::ptr<User> pUser = pSession->find<User>().where("user_id=?").bind(strUserId);

        if (!pUser)
		{
            response().out() << json_serializer(cppcms::http::response::not_found, action(), translate("指定用户不存在"));
			return;
		}

        response().out() << json_serializer(pUser->getArticles(), cppcms::http::response::ok, action(), translate("success!"));
	}
	catch (const std::exception& ex)
	{
		PLOG_ERROR << ex.what();
        response().out() << json_serializer(cppcms::http::response::internal_server_error, action(), translate(ex.what()));
	}
}

void ArticleService::all_article_by_user(const std::string &strUserId, int nPageSize, int nCurrentPage)
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

        response().out() << json_serializer(vecArticles, cppcms::http::response::ok, action(), translate("success!"));
	}
	catch (const std::exception& ex)
	{
		PLOG_ERROR << ex.what();
        response().out() << json_serializer(cppcms::http::response::internal_server_error, action(), translate(ex.what()));
	}
}

void ArticleService::all_article_by_category(const std::string& strCategoryId)
{
    try
    {
        const std::unique_ptr<dbo::Session>& pSession = dbo_session();
        dbo::Transaction transaction(*pSession);

        dbo::ptr<Category> pCategory = pSession->find<Category>().where("category_id=?").bind(strCategoryId);

        if (!pCategory)
        {
            response().out() << json_serializer(cppcms::http::response::not_found, action(), translate("没有找到相关分类"));
            return;
        }

        response().out() << json_serializer(pCategory->getArticles(), cppcms::http::response::ok, action(), translate("success!"));
    }
    catch (const std::exception& ex)
    {
        PLOG_ERROR << ex.what();
        response().out() << json_serializer(cppcms::http::response::internal_server_error, action(), translate(ex.what()));
    }
}

void ArticleService::all_article_by_category(const std::string& strCategoryId, int nPageSize, int nCurrentPage)
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

        response().out() << json_serializer(vecArticles, cppcms::http::response::ok, action(), translate("success!"));
	}
	catch (const std::exception& ex)
	{
		PLOG_ERROR << ex.what();
        response().out() << json_serializer(cppcms::http::response::internal_server_error, action(), translate(ex.what()));
	}
}
