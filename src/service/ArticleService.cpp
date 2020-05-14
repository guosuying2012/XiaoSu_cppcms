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

/**
* showdoc
* @catalog 文章管理
* @title 获取指定文章
* @description 通过文章ID获取该文章详细信息
* @method GET
* @url https://www.yengsu.com/xiaosu/web/api/v1/article/{ID}
* @return {"action": "/web/api/v1/article/{article_id}","msg": "获取成功","status": 200,"data": {}}
* @return_param action string 接口PATH
* @return_param msg string 服务器消息
* @return_param status int HTTP状态码
* @return_param data JsonObject 该文章详细信息
* @remark 文章对象请参阅数据字典, 状态码可以参阅全局状态码或者HTTP状态码.
*/
void ArticleService::article(const std::string& strArticleId)
{
    try
    {
        const std::unique_ptr<dbo::Session>& pSession = dbo_session();
        dbo::Transaction transaction(*pSession);
        dbo::ptr<Article> pArticle = nullptr;

        //获取博客
        if (request().request_method() == "GET")
        {
            pArticle = pSession->find<Article>()
                    .where("article_id=? and article_approval_status=1")
                    .bind(strArticleId);

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

        bool bIsAdmin = AuthorizeInstance::Instance().is_admin(strToken);

        pArticle = pSession->find<Article>()
                .where(bIsAdmin ? "article_id=?" : "article_id=? and article_approval_status=1")
                .bind(strArticleId);

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
                                            "未知错误!请联系管理员");
    }
}

/**
* showdoc
* @catalog 文章管理
* @title 添加博客
* @description 添加一条博客
* @method POST
* @url https://www.yengsu.com/xiaosu/web/api/v1/article
* @header authorization 必选 string 认证Token
* @json_param {"category_id":"0a3ed3e4-2d9d-4d02-a57a-2c5554d28ae4","article_title":"测试添加博客文章","article_cover":"文章封面","article_describe":"博客简介","article_content":"博客内容，测试测试测试测试测试测试测试测试测试测试测试"}
* @return {"action":"/web/api/v1/article","msg":"添加成功","status":201,"data": ""}
* @return_param action string URL_PATH
* @return_param msg string 服务器返回的消息
* @return_param status int HTTP状态码
* @return_param data JsonObject 添加成功的文章基础信息
* @remark 文章对象请参阅数据字典, 状态码可以参阅全局状态码或者HTTP状态码.
*/
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

        //判断有效性  不能只判空就认定对象有效
        if (!pArticle->category())
        {
	        response().status(response::not_found);
            response().out() << json_serializer(response::not_found,
							                    action(),
							                    "添加失败,未找到分类。");
            return;
        }

        const dbo::ptr<Category>& pCategory = dbo_session()->find<Category>()
                .where("category_id=?").bind(pArticle->category()->id());
        if (!pCategory)
        {
            response().status(response::not_found);
            response().out() << json_serializer(response::not_found,
                                                action(),
                                                "添加失败,未找到分类。");
            return;
        }

        //TOKEN合法
        const std::string& strUserId = AuthorizeInstance::Instance().get_user_id(strToken);
        const dbo::ptr<User>& pUser = pSession->find<User>()
                .where("user_id=?")
                .bind(strUserId)
                .where("user_status=?") //不仅存在，状态还必须正确
                .bind(UserStatus::ENABLE);
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

        //通过标题判断重复提交
        const Articles& articles = pSession->find<Article>()
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
        const dbo::ptr<Article>& pAddedPtr = pSession->add<Article>(pArticle);
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
    catch (const std::exception& ex)
    {
        PLOG_ERROR << ex.what();
	    response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error,
							                action(),
                                            "未知错误!请联系管理员");
    }
}

/**
* showdoc
* @catalog 文章管理
* @title 修改博客
* @description 修改指定ID的博客信息
* @method PUT
* @url https://www.yengsu.com/xiaosu/web/api/v1/article/{ID}
* @header authorization 必选 string 认证Token
* @json_param {"category_id": "1","article_title": "测试修改文章","article_cover": "封面地址","article_describe": "博客简介","article_content": "博客内容，测试测试测试测试测试测试测试测试测试测试测试"}
* @return {"action": "/web/api/v1/article/{article_id}","msg": "修改成功","status": 200,"data": {}}
* @return_param action string URL_PATH
* @return_param msg string 服务器返回的消息
* @return_param status int HTTP状态码
* @return_param data NULL 无
* @remark 文章对象请参阅数据字典, 状态码可以参阅全局状态码或者HTTP状态码.
*/
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
        /*if (pModify->category())//分类   不应该只是判空就认定分类对象有效
        {
            pArticle.modify()->category(pModify->category());
        }*/

        // 判断分类对象是否有效
        //===============================->
        //json解析对象时已经通过传过来的ID在数据库里面查询了一次，所以这里只需要判断是否为空就好
        if (pModify->category())
        {
            pArticle.modify()->category(pModify->category());
        }
        //<-===============================

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

        pArticle.modify()->update_last_change();
	    response().status(response::ok);
        response().out() << json_serializer(response::ok, action(), "修改成功");
    }
    catch (const std::exception& ex)
    {
        PLOG_ERROR << ex.what();
	    response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error,
							                action(),
                                            "未知错误!请联系管理员");
    }
}

/**
* showdoc
* @catalog 文章管理
* @title 删除博客
* @description 删除指定ID的博客信息
* @method DELETE
* @url https://www.yengsu.com/xiaosu/web/api/v1/article/{article_id}
* @header authorization 必选 string 认证Token
* @return {"action": "/web/api/v1/article/{article_id}","msg": "删除成功","status": 200,"data": {}}
* @return_param action string URL_PATH
* @return_param msg string 服务器返回的消息
* @return_param status int HTTP状态码
* @return_param data NULL 无
* @remark 文章对象请参阅数据字典, 状态码可以参阅全局状态码或者HTTP状态码.
*/
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
        pArticle.modify()->approval(false);
	    response().status(response::ok);
        response().out() << json_serializer(response::ok, action(), "删除成功");
    }
    catch (const std::exception& ex)
    {
        PLOG_ERROR << ex.what();
	    response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error,
							                action(),
                                            "未知错误!请联系管理员");
    }
}

/**
* showdoc
* @catalog 文章管理
* @title 移动博客
* @description 将文章移动到指定用户/分类下
* @method GET
* @url https://www.yengsu.com/xiaosu/web/api/v1/article/{article_id}/move_to/(user|category)/{user_id/category_id}
* @header authorization 必选 string 认证Token
* @return {"action": "/web/api/v1/article/{article_id}","msg": "移动成功","status": 200,"data": {}}
* @return_param action string URL_PATH
* @return_param msg string 服务器返回的消息
* @return_param status int HTTP状态码
* @return_param data NULL 无
* @remark 文章对象请参阅数据字典, 状态码可以参阅全局状态码或者HTTP状态码.
*/
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

    //管理员专用通道
    bIsAdmin = AuthorizeInstance::Instance().is_admin(strToken);

    try
    {
        const std::unique_ptr<dbo::Session>& pSession = dbo_session();
        dbo::Transaction transaction(*pSession);
        dbo::ptr<Article> pArticle = pSession->find<Article>()
                .where(bIsAdmin ? "article_id=?" : "article_id=? and article_approval_status=1")
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
            dbo::ptr<User> pUser = pSession->find<User>()
                    .where("user_id=?")
                    .bind(strId)
                    .where("user_status=?")
                    .bind(UserStatus::ENABLE);
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
                                            "未知错误!请联系管理员");
    }
}

/**
* showdoc
* @catalog 文章管理
* @title 获取所有文章
* @description 此接口返回所有状态正常的博客
* @method GET
* @url https://www.yengsu.com/xiaosu/web/api/v1/article/all_articles
* @return {"action":"/web/api/v1/article/all_articles","msg":"获取成功","status":200,"data":[]}
* @return_param action string 接口PATH
* @return_param msg string 服务器消息
* @return_param status int HTTP状态码
* @return_param data JsonArray 文章列表
* @remark 文章对象请参阅数据字典, 状态码可以参阅全局状态码或者HTTP状态码.
*/
void ArticleService::all_articles()
{
    bool bIsAdmin = false;
    const std::string& strToken = request().getenv("HTTP_AUTHORIZATION");

    if (!strToken.empty())
    {
        const std::pair<bool, cppcms::string_key>& verify = AuthorizeInstance::Instance().verify_token(strToken);
        if (verify.first)
        {
            bIsAdmin = AuthorizeInstance::Instance().is_admin(strToken);
        }
    }

	try
	{
		const std::unique_ptr<dbo::Session>& pSession = dbo_session();
		dbo::Transaction transaction(*pSession);
		Articles vecArticles = pSession->find<Article>()
		        .where(bIsAdmin ? "" : "article_approval_status=1");
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
                                            "未知错误!请联系管理员");
	}
}

/**
* showdoc
* @catalog 文章管理
* @title 分页获取所有文章
* @description 此接口返回所有状态正常的博客
* @method GET
* @url https://www.yengsu.com/xiaosu/web/api/v1/article/all_articles/page/{PageSize}/{CurrentPage}
* @return {"action":"/web/api/v1/article/all_articles","msg":"获取成功","status":200,"data":[]}
* @return_param action string 接口PATH
* @return_param msg string 服务器消息
* @return_param status int HTTP状态码
* @return_param data JsonArray 文章列表
* @remark 文章对象请参阅数据字典, 状态码可以参阅全局状态码或者HTTP状态码.
*/
void ArticleService::all_articles(int nPageSize, int nCurrentPage)
{
    bool bIsAdmin = false;
    const std::string& strToken = request().getenv("HTTP_AUTHORIZATION");

    if (!strToken.empty())
    {
        const std::pair<bool, cppcms::string_key>& verify = AuthorizeInstance::Instance().verify_token(strToken);
        if (verify.first)
        {
            bIsAdmin = AuthorizeInstance::Instance().is_admin(strToken);
        }
    }

	try
	{
		const std::unique_ptr<dbo::Session>& pSession = dbo_session();
		dbo::Transaction transaction(*pSession);
		Articles vecArticles = pSession->find<Article>()
		        .where(bIsAdmin ? "" : "article_approval_status=1")
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
                                            "未知错误!请联系管理员");
	}
}

/**
* showdoc
* @catalog 文章管理
* @title 获取指定用户的博客
* @description 此接口返回指定用户所有状态正常的博客
* @method GET
* @url https://www.yengsu.com/xiaosu/web/api/v1/article/all_article_by_user/{user_id}
* @return {"action":"/web/api/v1/article/all_articles","msg":"获取成功","status":200,"data":[]}
* @return_param action string 接口PATH
* @return_param msg string 服务器消息
* @return_param status int HTTP状态码
* @return_param data JsonArray 文章列表
* @remark 文章对象请参阅数据字典, 状态码可以参阅全局状态码或者HTTP状态码.
*/
void ArticleService::all_article_by_user(const std::string& strUserId)
{
    bool bIsAdmin = false;
    std::vector<dbo::ptr<Article> > vecArticles;
    const std::string& strToken = request().getenv("HTTP_AUTHORIZATION");

    if (!strToken.empty())
    {
        const std::pair<bool, cppcms::string_key>& verify = AuthorizeInstance::Instance().verify_token(strToken);
        if (verify.first)
        {
            bIsAdmin = AuthorizeInstance::Instance().is_admin(strToken);
        }
    }

	try
	{
		const std::unique_ptr<dbo::Session>& pSession = dbo_session();
		dbo::Transaction transaction(*pSession);
        dbo::ptr<User> pUser = pSession->find<User>()
                .where("user_id=?")
                .where(bIsAdmin ? "" : "user_status=0")
                .bind(strUserId);

        // 将异常状态的文章排除，前提是非管理员
        //===============================->
        if (!bIsAdmin)
        {
            vecArticles.clear();
            const Articles& articles = pUser->getArticles();
            for (const dbo::ptr<Article>& item : articles)
            {
                if (!item->approval())
                {
                    continue;
                }
                vecArticles.push_back(item);
            }
        }
        //<-===============================

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
                                            "未知错误!请联系管理员");
	}
}

/**
* showdoc
* @catalog 文章管理
* @title 分页获取指定用户的博客
* @description 此接口返回指定用户所有状态正常的博客
* @method GET
* @url https://www.yengsu.com/xiaosu/web/api/v1/article/all_article_by_user/page/{user_id}/{PageSize}/{CurrentPage}
* @return {"action":"/web/api/v1/article/all_articles","msg":"获取成功","status":200,"data":[]}
* @return_param action string 接口PATH
* @return_param msg string 服务器消息
* @return_param status int HTTP状态码
* @return_param data JsonArray 文章列表
* @remark 文章对象请参阅数据字典, 状态码可以参阅全局状态码或者HTTP状态码.
*/
void ArticleService::all_article_by_user(const std::string& strUserId,
                                         int nPageSize,
                                         int nCurrentPage)
{
    bool bIsAdmin = false;
    std::vector<dbo::ptr<Article> > vecArticles;
    const std::string& strToken = request().getenv("HTTP_AUTHORIZATION");

    if (!strToken.empty())
    {
        const std::pair<bool, cppcms::string_key>& verify = AuthorizeInstance::Instance().verify_token(strToken);
        if (verify.first)
        {
            bIsAdmin = AuthorizeInstance::Instance().is_admin(strToken);
        }
    }

	try
	{
		const std::unique_ptr<dbo::Session>& pSession = dbo_session();
        dbo::Transaction transaction(*pSession);
        dbo::ptr<User> pUser = pSession->find<User>()
                .where("user_id=?")
                .where(bIsAdmin ? "" : "user_status=0")
                .bind(strUserId)
                .offset((nCurrentPage -1 ) * nPageSize)
                .limit(nPageSize);

        // 将异常状态的文章排除，前提是非管理员
        //===============================->
        if (!bIsAdmin)
        {
            vecArticles.clear();
            const Articles& articles = pUser->getArticles();
            for (const dbo::ptr<Article>& item : articles)
            {
                if (!item->approval())
                {
                    continue;
                }
                vecArticles.push_back(item);
            }
        }
        //<-===============================

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
							                "未知错误!请联系管理员");
	}
}

/**
* showdoc
* @catalog 文章管理
* @title 分页获取指定分类的博客
* @description 此接口返回指定分类所有状态正常的博客
* @method GET
* @url https://www.yengsu.com/xiaosu/web/api/v1/article/all_article_by_category
* @return {"action":"/web/api/v1/article/all_articles","msg":"获取成功","status":200,"data":[]}
* @return_param action string 接口PATH
* @return_param msg string 服务器消息
* @return_param status int HTTP状态码
* @return_param data JsonArray 文章列表
* @remark 文章对象请参阅数据字典, 状态码可以参阅全局状态码或者HTTP状态码.
*/
void ArticleService::all_article_by_category(const std::string& strCategoryId)
{
    bool bIsAdmin = false;
    std::vector<dbo::ptr<Article> > vecArticles;
    const std::string& strToken = request().getenv("HTTP_AUTHORIZATION");

    if (!strToken.empty())
    {
        const std::pair<bool, cppcms::string_key>& verify = AuthorizeInstance::Instance().verify_token(strToken);
        if (verify.first)
        {
            bIsAdmin = AuthorizeInstance::Instance().is_admin(strToken);
        }
    }

    try
    {
        const std::unique_ptr<dbo::Session>& pSession = dbo_session();
        dbo::Transaction transaction(*pSession);

        dbo::ptr<Category> pCategory = pSession->find<Category>()
                .where("category_id=?")
                .bind(strCategoryId);

        // 将异常状态的文章排除，前提是非管理员
        //===============================->
        if (!bIsAdmin)
        {
            vecArticles.clear();
            const Articles& articles = pCategory->getArticles();
            for (const dbo::ptr<Article>& item : articles)
            {
                if (!item->approval())
                {
                    continue;
                }
                vecArticles.push_back(item);
            }
        }
        //<-===============================

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
							                "未知错误!请联系管理员");
    }
}

/**
* showdoc
* @catalog 文章管理
* @title 分页获取指定分类的博客
* @description 此接口返回指定分类所有状态正常的博客
* @method GET
* @url https://www.yengsu.com/xiaosu/web/api/v1/article/all_article_by_category/page/{category_id}/{PageSize}/{CurrentPage}
* @return {"action":"/web/api/v1/article/all_articles","msg":"获取成功","status":200,"data":[]}
* @return_param action string 接口PATH
* @return_param msg string 服务器消息
* @return_param status int HTTP状态码
* @return_param data JsonArray 文章列表
* @remark 文章对象请参阅数据字典, 状态码可以参阅全局状态码或者HTTP状态码.
*/
void ArticleService::all_article_by_category(const std::string& strCategoryId,
                                             int nPageSize,
                                             int nCurrentPage)
{
    bool bIsAdmin = false;
    const std::string& strToken = request().getenv("HTTP_AUTHORIZATION");

    // 这里是为了判定是否是管理员（是管理员就无视异常状态，不是管理员就排除异常状态）
    //===============================->
    if (!strToken.empty())
    {
        const std::pair<bool, cppcms::string_key>& verify = AuthorizeInstance::Instance().verify_token(strToken);
        if (verify.first)
        {
            bIsAdmin = AuthorizeInstance::Instance().is_admin(strToken);
        }
    }
    //<-===============================

	try
	{
		const std::unique_ptr<dbo::Session>& pSession = dbo_session();
		dbo::Transaction transaction(*pSession);
		const Articles& vecArticles =  pSession->find<Article>()
		        .where("category_id=?")
		        .bind(strCategoryId)
		        .where("article_approval_status=1")
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
                                            "未知错误!请联系管理员");
	}
}
