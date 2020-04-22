//
// Created by yengsu on 2020/3/1.
//

#include "ArticleService.h"

#include <exception>

#include <cppcms/http_request.h>
#include <cppcms/http_response.h>
#include <cppcms/url_dispatcher.h>

#include "define.h"
#include "model/user.h"
#include "model/article.h"
#include "model/userinfo.h"
#include "model/category.h"
#include "utils/JsonSerializer.h"
#include "utils/JsonDeserializer.h"
#include "utils/AuthorizeInstance.h"

using cppcms::http::response;

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
	            response().status(response::not_found);
                response().out() << json_serializer(response::not_found,
                		                            action(),
                		                            "未找到相关内容");
                return;
            }
	        response().status(response::ok);
            response().out() << json_serializer(pArticle,
            		                            response::ok,
            		                            action(),
            		                            "获取成功");
            return;
        }

        //以下操作均为敏感操作，加上用户授权验证
        //首先获取HTTP HEADER 有没有TOKEN
        const cppcms::string_key& strToken = request().getenv("HTTP_AUTHORIZATION");
        if (strToken.empty())
        {
            //提示用户要带上TOKEN  bad_request
	        response().status(response::bad_request);
            response().out() << json_serializer(response::bad_request,
							                    action(),
							                    "认证失败");
            return;
        }

        //添加
        if (request().request_method() == "POST")
        {
            if (request().raw_post_data().second <= 0)
            {
	            response().status(response::precondition_failed);
                response().out() << json_serializer(response::precondition_failed,
                                                    action(),
                                                    "添加失败,未收到需要添加的内容。");
                return;
            }
            add_article();
            return;
        }

        //修改
        if (request().request_method() == "PUT")
        {
            if (request().raw_post_data().second <= 0)
            {
	            response().status(response::precondition_failed);
                response().out() << json_serializer(response::precondition_failed,
							                        action(),
							                        "修改失败");
                return;
            }
            modify_article(pArticle);
            return;
        }

        //删除
        if (request().request_method() == "DELETE")
        {
            delete_article(pArticle);
            return;
        }
    }
    catch (const std::exception& ex)
    {
        PLOG_ERROR << ex.what();
	    response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error,
							                action(),
							                ex.what());
    }
}

void ArticleService::add_article()
{
    bool bIsAdmin = false;

    //来到这里说名用户给了TOKEN，那么先校验TOKEN合法性
    const std::string& strToken = request().getenv("HTTP_AUTHORIZATION");
    const std::pair<bool, cppcms::string_key>& verify = AuthorizeInstance::Instance().verify_token(strToken);
    if (!verify.first)
    {
        //捕捉到TOKEN不合法，不能放行
        PLOG_ERROR << verify.second.str();
	    response().status(response::unauthorized);
        response().out() << json_serializer(response::unauthorized,
							                action(),
							                verify.second);
        return;
    }

    try
    {
        const std::unique_ptr<dbo::Session>& pSession = dbo_session();
        dbo::Transaction transaction(*pSession);
        dbo::ptr<Article> pArticle = dbo::make_ptr<Article>();

        char strBuffer[request().raw_post_data().second + 1];
        memcpy(strBuffer, static_cast<char*>(request().raw_post_data().first),
               request().raw_post_data().second);
        json_unserializer(strBuffer, pArticle);

        //判断有效性
        if (!pArticle->category())
        {
	        response().status(response::not_found);
            response().out() << json_serializer(response::not_found,
							                    action(),
							                    "添加失败,未找到分类。");
            return;
        }

        //管理员专用通道
        bIsAdmin = AuthorizeInstance::Instance().is_admin(strToken);

        if (!bIsAdmin)
        {
            //TOKEN合法
            const std::string& strUserId = AuthorizeInstance::Instance().get_user_id(strToken);
            //const dbo::ptr<User>& pUser = pSession->find<User>().where("user_id=?").bind(strUserId);
            const dbo::ptr<User>& pUser = pSession->load<User>(strUserId);
            //用户应该是TOKEN里面的
            if (pUser)
            {
                pArticle.modify()->user(pUser);
            }
            else
            {
                //如果通过TOKEN没找到用户说明有入侵
                PLOG_ERROR << "请注意，有违规操作！！！"
                           << "Token: " << strToken
                           << ",UserId : " << strUserId;
	            response().status(response::ok);
                response().out() << json_serializer(response::ok,
							                        action(),
							                        "检测违规操作");
                return;
            }
        }

        //通过标题判断重复提交
        Articles articles = pSession->find<Article>()
                .where("article_title=?")
        		.bind(pArticle->title());
        if (!articles.empty())
        {
	        response().status(response::found);
            response().out() << json_serializer(response::found,
							                    action(),
							                    "添加失败,博客已存在");
            return;
        }

        //判断添加是否成功
        dbo::ptr<Article> pAddedPtr = pSession->add<Article>(pArticle);
        if (pAddedPtr)
        {
	        response().status(response::created);
            response().out() << json_serializer(pAddedPtr,
							                    response::created,
							                    action(),
							                    "添加成功");
        }
        else
        {
	        response().status(response::precondition_failed);
            response().out() << json_serializer(response::precondition_failed,
							                    action(),
							                    "添加失败");
        }
    }
    catch (const dbo::ObjectNotFoundException& ex)
    {
        PLOG_DEBUG << ex.what();
	    response().status(response::not_found);
        response().out() << json_serializer(response::not_found,
							                action(),
							                "未找到相关记录");
    }
    catch (const std::exception& ex)
    {
        PLOG_ERROR << ex.what();
	    response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error,
							                action(),
							                ex.what());
    }
}

void ArticleService::modify_article(dbo::ptr<Article>& pArticle)
{
    bool bIsAdmin = false;

    //来到这里说名用户给了TOKEN，那么先校验TOKEN合法性
    const std::string& strToken = request().getenv("HTTP_AUTHORIZATION");
    const std::pair<bool, cppcms::string_key>& verify = AuthorizeInstance::Instance().verify_token(strToken);
    if (!verify.first)
    {
        //捕捉到TOKEN不合法，不能放行
        PLOG_ERROR << verify.second.str();
	    response().status(response::unauthorized);
        response().out() << json_serializer(response::unauthorized,
							                action(),
							                verify.second);
        return;
    }

    if (!pArticle)
    {
        //失败，未找到相关
	    response().status(response::not_found);
        response().out() << json_serializer(response::not_found,
							                action(),
							                "修改失败,未找到该博文");
        return;
    }

    //管理员专用通道
    bIsAdmin = AuthorizeInstance::Instance().is_admin(strToken);

    if (!bIsAdmin)
    {
        //验证博文是否归属于当前操作用户
        const std::string& strUserId = AuthorizeInstance::Instance().get_user_id(strToken);
        if (strUserId != pArticle->user().id())
        {
            //无权操作别人的博文
            PLOG_ERROR << "有用户在执行修改操作时不属于他的文章：" << strUserId << "Token: " << strToken;
	        response().status(response::unauthorized);
            response().out() << json_serializer(response::unauthorized,
							                    action(),
							                    "权限不足");
            return;
        }
    }

    try
    {
        char strBuffer[request().raw_post_data().second + 1];
        memcpy(strBuffer, static_cast<char*>(request().raw_post_data().first),
               request().raw_post_data().second);
        dbo::ptr<Article> pModify = dbo::make_ptr<Article>();
        json_unserializer(strBuffer, pModify);

        //不应该由用户修改博客作者
        /*if (pModify->user())//用户
        {
            pArticle.modify()->user(pModify->user());
        }*/
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

	    response().status(response::ok);
        response().out() << json_serializer(response::ok, action(), "修改成功");
    }
    catch (const std::exception& ex)
    {
        PLOG_ERROR << ex.what();
	    response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error,
							                action(),
							                ex.what());
    }
}

void ArticleService::delete_article(dbo::ptr<Article>& pArticle)
{
    bool bIsAdmin = false;

    //来到这里说名用户给了TOKEN，那么先校验TOKEN合法性
    const std::string& strToken = request().getenv("HTTP_AUTHORIZATION");
    const std::pair<bool, cppcms::string_key>& verify = AuthorizeInstance::Instance().verify_token(strToken);
    if (!verify.first)
    {
        //捕捉到TOKEN不合法，不能放行
        PLOG_ERROR << verify.second.str();
	    response().status(response::unauthorized);
        response().out() << json_serializer(response::unauthorized,
							                action(),
							                verify.second);
        return;
    }

    if (!pArticle)
    {
        //失败，未找到相关
	    response().status(response::not_found);
        response().out() << json_serializer(response::not_found,
							                action(),
							                "删除失败,未找到博文.");
        return;
    }

    //管理员专用通道
    bIsAdmin = AuthorizeInstance::Instance().is_admin(strToken);

    if (!bIsAdmin)
    {
        //验证博文是否归属于当前操作用户
        const std::string& strUserId = AuthorizeInstance::Instance().get_user_id(strToken);
        if (strUserId != pArticle->user().id())
        {
            //无权操作别人的博文
            PLOG_ERROR << "有用户在执行删除操作时不属于他的文章：" << strUserId << "Token: " << strToken;
	        response().status(response::unauthorized);
            response().out() << json_serializer(response::unauthorized,
							                    action(),
							                    "权限不足");
            return;
        }
    }

    try
    {
        pArticle.remove();
	    response().status(response::ok);
        response().out() << json_serializer(response::ok, action(), "删除成功");
    }
    catch (const std::exception& ex)
    {
        PLOG_ERROR << ex.what();
	    response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error,
							                action(),
							                ex.what());
    }
}

void ArticleService::move_to(const std::string& strArticleId,
                             const std::string& strMoveTo,
                             const std::string& strId)
{
    bool bIsAdmin = false;

    //以下操作均为敏感操作，加上用户授权验证
    //首先获取HTTP HEADER 有没有TOKEN
    const std::string& strToken = request().getenv("HTTP_AUTHORIZATION");
    if (strToken.empty())
    {
        //提示用户要带上TOKEN  bad_request
	    response().status(response::unauthorized);
        response().out() << json_serializer(response::unauthorized,
							                action(),
							                "请先登陆");
        return;
    }

    //来到这里说明用户给了TOKEN，那么先校验TOKEN合法性
    const std::pair<bool, cppcms::string_key>& verify = AuthorizeInstance::Instance().verify_token(strToken);
    if (!verify.first)
    {
        //捕捉到TOKEN不合法，不能放行
        PLOG_ERROR << verify.second.str();
	    response().status(response::unauthorized);
        response().out() << json_serializer(response::unauthorized,
							                action(),
							                verify.second);
        return;
    }

    try
    {
        const std::unique_ptr<dbo::Session>& pSession = dbo_session();
        dbo::Transaction transaction(*pSession);
        dbo::ptr<Article> pArticle = pSession->find<Article>()
                .where("article_id=?")
                .bind(strArticleId);

        if (!pArticle)
        {
            //失败，未找到相关
	        response().status(response::not_found);
            response().out() << json_serializer(response::not_found,
							                    action(),
							                    "移动失败,未找到博文");
            return;
        }

        //管理员专用通道
        bIsAdmin = AuthorizeInstance::Instance().is_admin(strToken);

        if (!bIsAdmin)
        {
            //验证博文是否归属于当前操作用户
            const std::string& strUserId = AuthorizeInstance::Instance().get_user_id(strToken);
            if (strUserId != pArticle->user().id())
            {
                //无权操作别人的博文
                PLOG_ERROR << "有用户在执行移动操作时不属于他的文章：" << strUserId << "Token: " << strToken;
	            response().status(response::unauthorized);
                response().out() << json_serializer(response::unauthorized,
							                        action(),
							                        "权限不足");
                return;
            }
        }

        if (strMoveTo == "user")
        {
            dbo::ptr<User> pUser = pSession->find<User>().where("user_id=?").bind(strId);
            //dbo::ptr<User> pUser = pSession->load<User>(strId);
            if (!pUser)
            {
                //未找到相关
	            response().status(response::not_found);
                response().out() << json_serializer(response::not_found,
							                        action(),
							                        "移动失败,未找到用户");
                return;
            }

            pArticle.modify()->user(pUser);
        }

        if (strMoveTo == "category")
        {
            dbo::ptr<Category> pCategory = pSession->find<Category>()
                    .where("category_id=?")
                    .bind(strId);
            if (!pCategory)
            {
                //未找到相关
	            response().status(response::not_found);
                response().out() << json_serializer(response::not_found,
							                        action(),
							                        "移动失败,未找到分类");
                return;
            }

            pArticle.modify()->category(pCategory);
        }

	    response().status(response::ok);
        response().out() << json_serializer(response::ok, action(), "移动成功");
    }
    catch (const std::exception& ex)
    {
        PLOG_ERROR << ex.what();
	    response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error,
							                action(),
							                ex.what());
    }
}

void ArticleService::all_articles()
{
	try
	{
		const std::unique_ptr<dbo::Session>& pSession = dbo_session();
		dbo::Transaction transaction(*pSession);
		Articles vecArticles = pSession->find<Article>();
		response().status(response::ok);
        response().out() << json_serializer(vecArticles,
							                response::ok,
							                action(),
							                "获取成功");
	}
	catch (const std::exception& ex)
	{
		PLOG_ERROR << ex.what();
		response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error,
							                action(),
							                ex.what());
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

		response().status(response::ok);
        response().out() << json_serializer(vecArticles,
							                response::ok,
							                action(),
							                "获取成功");
	}
	catch (const std::exception& ex)
	{
		PLOG_ERROR << ex.what();
		response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error,
							                action(),
							                ex.what());
	}
}

void ArticleService::all_article_by_user(const std::string& strUserId)
{
	try
	{
		const std::unique_ptr<dbo::Session>& pSession = dbo_session();
		dbo::Transaction transaction(*pSession);

        dbo::ptr<User> pUser = pSession->load<User>(strUserId);

		response().status(response::ok);
        response().out() << json_serializer(pUser->getArticles(),
							                response::ok,
							                action(),
							                "获取成功");
	}
    catch (const dbo::ObjectNotFoundException& ex)
    {
        PLOG_ERROR << ex.what();
	    response().status(response::not_found);
        response().out() << json_serializer(response::not_found,
							                action(),
							                "指定用户不存在");
    }
	catch (const std::exception& ex)
	{
		PLOG_ERROR << ex.what();
		response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error,
							                action(),
							                ex.what());
	}
}

void ArticleService::all_article_by_user(const std::string& strUserId,
                                         int nPageSize,
                                         int nCurrentPage)
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

		response().status(response::ok);
        response().out() << json_serializer(vecArticles,
							                response::ok,
							                action(),
							                "获取成功");
	}
	catch (const std::exception& ex)
	{
		PLOG_ERROR << ex.what();
		response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error,
							                action(),
							                ex.what());
	}
}

void ArticleService::all_article_by_category(const std::string& strCategoryId)
{
    try
    {
        const std::unique_ptr<dbo::Session>& pSession = dbo_session();
        dbo::Transaction transaction(*pSession);

        dbo::ptr<Category> pCategory = pSession->load<Category>(strCategoryId);

	    response().status(response::ok);
        response().out() << json_serializer(pCategory->getArticles(),
							                response::ok,
							                action(),
							                "获取成功");
    }
    catch (const dbo::ObjectNotFoundException& ex)
    {
        PLOG_ERROR << ex.what();
	    response().status(response::not_found);
        response().out() << json_serializer(response::not_found,
							                action(),
							                "没有找到相关分类");
    }
    catch (const std::exception& ex)
    {
        PLOG_ERROR << ex.what();
	    response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error,
							                action(),
							                ex.what());
    }
}

void ArticleService::all_article_by_category(const std::string& strCategoryId,
                                             int nPageSize,
                                             int nCurrentPage)
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

		response().status(response::ok);
        response().out() << json_serializer(vecArticles,
							                response::ok,
							                action(),
							                "获取成功");
	}
	catch (const std::exception& ex)
	{
		PLOG_ERROR << ex.what();
		response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error,
							                action(),
							                ex.what());
	}
}
