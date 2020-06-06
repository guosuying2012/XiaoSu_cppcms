#include "CategoryService.h"
#include "define.h"
#include "model/category.h"
#include "model/article.h"
#include "model/user.h"
#include "model/userinfo.h"
#include "utils/JsonSerializer.h"
#include "utils/JsonDeserializer.h"
#include "utils/AuthorizeInstance.h"
#include <cppcms/url_dispatcher.h>
#include <cppcms/http_response.h>
#include <cppcms/http_request.h>

using cppcms::http::response;
CategoryService::CategoryService(cppcms::service& srv)
    :BaseService(srv)
{
    dispatcher().map("GET", "", &CategoryService::category_list, this);
    dispatcher().map("POST", "", &CategoryService::add_category, this);
    dispatcher().map("PUT", "/(.*)", &CategoryService::modify_category, this, 1);
    dispatcher().map("DELETE", "/(.*)", &CategoryService::delete_category, this, 1);
}

void CategoryService::add_category()
{
    // 判断请求方式
    //===============================->
    if (request().request_method() != "POST")
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request, action(), "错误的请求");
        return;
    }
    //<-===============================

    // 此接口仅超级管理员，首先验证TOKEN
    //===============================->
    const cppcms::string_key& strToken = request().getenv("HTTP_AUTHORIZATION");
    if (strToken.empty())
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request,action(),"认证失败");
        return;
    }

    const std::pair<bool, std::string> verify = AuthorizeInstance::Instance().verify_token(strToken);
    if (!verify.first)
    {
        response().status(response::unauthorized);
        response().out() << json_serializer(response::unauthorized, action(), "认证失败");
        return;
    }
    //<-===============================

    // 只能管理员或者超级管理员
    //===============================->
    const UserRole nRole = (UserRole)AuthorizeInstance::Instance().get_role(strToken);
    if (nRole != UserRole::MEMBER && nRole != UserRole::ADMINISTRATOR)
    {
        response().status(response::not_modified);
        response().out() << json_serializer(response::not_modified,action(),"权限不足");;
        return;
    }
    //<-===============================

    try
    {
        // 获取请求体，请求体应该是分类的实体
        //===============================->
        const std::unique_ptr<dbo::Session>& pSession = dbo_session();
        dbo::Transaction transaction(*pSession);
        dbo::ptr<Category> pCategory = nullptr;

        const std::pair<void*, size_t> content = request().raw_post_data();
        char* strBuffer = new char[content.second];
        memcpy(strBuffer, content.first, content.second);
        strBuffer[content.second] = '\0';
        json_unserializer(strBuffer, pCategory);
        delete[] strBuffer;
        //<-===============================

        // 验证请求合法性
        //===============================->
        if (pCategory->get_name().empty())
        {
            response().status(response::bad_request);
            response().out() << json_serializer(response::bad_request, action(), "类别名称不能为空");
            return;
        }

        // 排序合法吗？
        if (pCategory->rank() < 0)
        {
            response().status(response::bad_request);
            response().out() << json_serializer(response::bad_request, action(), "排序不可以小于0");
            return;
        }

        Categorys pTemps;
        if (!pCategory->get_parent_id().empty() && pCategory->get_parent_id() != "0")
        {
            // 父类别存在吗？
            pTemps = pSession->find<Category>().where("category_id=?").bind(pCategory->get_parent_id());
            if (pTemps.empty())
            {
                response().status(response::bad_request);
                response().out() << json_serializer(response::bad_request, action(), "指定的父类别不存在");
                return;
            }
        }

        // 类别名称存在 & 合法吗？
        pTemps = pSession->find<Category>().where("category_name=?").bind(pCategory->get_name());
        if (!pTemps.empty())
        {
            response().status(response::bad_request);
            response().out() << json_serializer(response::bad_request, action(), "该类别已经存在");
            return;
        }
        //<-===============================

        // 返回内容
        //===============================->
        dbo::ptr<Category> pAddedCategory = pSession->add<Category>(pCategory);
        if (pAddedCategory)
        {
            response().status(response::created);
            response().out() << json_serializer(pAddedCategory, response::created, action(), "类别创建成功");
            return;
        }

        response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error, action(), "类别创建失败");
        return;
        //<-===============================
    }
    catch (const std::exception& ex)
    {
        PLOG_DEBUG << ex.what();
        response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error, action(), "服务器内部出错");
        return;
    }

}

void CategoryService::delete_category(const std::string &strId)
{
    // 判断请求方式
    //===============================->
    if (request().request_method() != "DELETE")
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request, action(), "错误的请求");
        return;
    }
    //<-===============================

    if (strId.empty())
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request,action(),"无效ID");
        return;
        return;
    }

    // 此接口仅超级管理员，首先验证TOKEN
    //===============================->
    const cppcms::string_key& strToken = request().getenv("HTTP_AUTHORIZATION");
    if (strToken.empty())
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request,action(),"认证失败");
        return;
    }
    const std::pair<bool, std::string> verify = AuthorizeInstance::Instance().verify_token(strToken);
    if (!verify.first)
    {
        response().status(response::unauthorized);
        response().out() << json_serializer(response::unauthorized, action(), "认证失败");
        return;
    }
    const bool bAdmin = AuthorizeInstance::Instance().is_admin(strToken);
    if (!bAdmin)
    {
        response().status(response::unauthorized);
        response().out() << json_serializer(response::unauthorized,action(),"权限不足");
        return;
    }
    //<-===============================

    try
    {
        const std::unique_ptr<dbo::Session> &pSession = dbo_session();
        dbo::Transaction transaction(*pSession);

        // 查找分类是否存在
        //===============================->
        dbo::ptr<Category> pCategory = pSession->find<Category>().where("category_id=?").bind(strId);
        if (!pCategory)
        {
            response().status(response::not_found);
            response().out() << json_serializer(response::not_found,action(),"未找到该类别");
            return;
        }
        //<-===============================

        // 分类存在，查找是否有所属文章
        //===============================->
        if (pCategory->getArticles().size() > 0)
        {
            response().status(response::bad_request);
            response().out() << json_serializer(response::bad_request,action(),"该类别下有文章，不能执行删除，请先处理。");
            return;
        }
        //<-===============================

        // 所属文章已经被处理，执行删除
        //===============================->
        pCategory.remove();
        response().status(response::ok);
        response().out() << json_serializer(response::ok,action(),"类别删除成功");
        return;
        //<-===============================
    }
    catch (const std::exception& ex)
    {
        response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error,action(),"未知错误");
        return;
    }
}

void CategoryService::modify_category(const std::string &strId)
{
    // 判断请求方式
    //===============================->
    if (request().request_method() != "PUT")
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request, action(), "错误的请求");
        return;
    }
    //<-===============================

    // 此接口仅超级管理员，首先验证TOKEN
    //===============================->
    const cppcms::string_key& strToken = request().getenv("HTTP_AUTHORIZATION");
    if (strToken.empty())
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request,action(),"认证失败");
        return;
    }
    const std::pair<bool, std::string> verify = AuthorizeInstance::Instance().verify_token(strToken);
    if (!verify.first)
    {
        response().status(response::unauthorized);
        response().out() << json_serializer(response::unauthorized, action(), "认证失败");
        return;
    }
    //<-===============================

    // 修改请求，只能管理员或者超级管理员修改
    //===============================->
    const UserRole nRole = (UserRole)AuthorizeInstance::Instance().get_role(strToken);
    if (nRole != UserRole::MEMBER && nRole != UserRole::ADMINISTRATOR)
    {
        response().status(response::not_modified);
        response().out() << json_serializer(response::not_modified,action(),"权限不足");;
        return;
    }
    //<-===============================

    try
    {
        if (strId.empty())
        {
            response().status(response::not_found);
            response().out() << json_serializer(response::not_found,action(),"未找到该类别");
            return;
        }

        dbo::ptr<Category> pCategory = dbo::make_ptr<Category>();
        const cppcms::string_key& strBuffer = static_cast<char *>(request().raw_post_data().first);
        std::cout << strBuffer << std::endl;
        json_unserializer(strBuffer, pCategory);

        // 验证请求合法性
        //===============================->
        const std::unique_ptr<dbo::Session> &pSession = dbo_session();
        dbo::Transaction transaction(*pSession);

        dbo::ptr<Category> category = nullptr;
        category = pSession->find<Category>().where("category_id=?").bind(strId);
        if (!category)
        {
            response().status(response::not_found);
            response().out() << json_serializer(response::not_found,action(),"未找到该类别");
            return;
        }

        if (pCategory->get_name().empty())
        {
            response().status(response::not_found);
            response().out() << json_serializer(response::not_found,action(),"类别名不能为空");
            return;
        }

        if (pCategory->rank() < 0)
        {
            response().status(response::not_found);
            response().out() << json_serializer(response::not_found,action(),"级别不能为负数");
            return;
        }

        // 合法性验证完成，开始修改信息
        //===============================->
        category.modify()->rank(pCategory->rank());
        category.modify()->set_name(pCategory->get_name());
        //<-===============================

        if (!pCategory->get_parent_id().empty() || pCategory->get_parent_id() == "0")
        {
            category = pSession->find<Category>().where("category_id=?").bind(pCategory->get_parent_id());
            if (category)
            {
                category.modify()->set_parent_id(pCategory->get_parent_id());
            }
        }
        //<-===============================

        response().status(response::ok);
        response().out() << json_serializer(response::ok,action(),"修改成功");
        return;
    }
    catch (const std::exception& ex)
    {
        response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error,action(),"未知错误");
        return;
    }
}

void CategoryService::category_list()
{
    // 判断请求方式
    //===============================->
    if (request().request_method() != "GET")
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request, action(), "错误的请求");
        return;
    }
    //<-===============================

    // 应该是所有用户都有的权限
    //===============================->
    try
    {
        const std::unique_ptr<dbo::Session>& pSession = dbo_session();
        dbo::Transaction transaction(*pSession);

        const Categorys categorys = pSession->find<Category>();
        const std::string& strJson = json_serializer(categorys, response::ok, action(), "获取成功");

        // 去掉不需要的信息以减少响应体
        //===============================->
        rapidjson::Document document;
        if (document.Parse(strJson).HasParseError())
        {
            response().status(response::internal_server_error);
            response().out() << json_serializer(response::internal_server_error, action(), "未知错误");
            return;
        }

        if (document.HasMember("data") && document["data"].IsArray())
        {
            rapidjson::Value &data = document["data"];
            for (rapidjson::SizeType i = 0; i < data.Size(); ++i)
            {
                rapidjson::Value &article = data[i];
                article.RemoveMember("articles");
            }
        }
        //<-===============================

        // 返回JSON数据
        //===============================->
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        document.Accept(writer);

        response().status(response::ok);
        response().out() << buffer.GetString();
        //<-===============================

    }
    catch (const std::exception& ex)
    {
        response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error, action(), "服务器内部出错");
        return;
    }
    //<-===============================
}
