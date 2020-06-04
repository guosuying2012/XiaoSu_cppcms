//
// Created by yengsu on 2020/4/21.
//

#include "UserService.h"
#include "model/user.h"
#include "model/userinfo.h"
#include "model/article.h"
#include "utils/AuthorizeInstance.h"
#include "utils/JsonSerializer.h"
#include "utils/JsonDeserializer.h"
#include <cppcms/url_dispatcher.h>
#include <cppcms/http_response.h>
#include <cppcms/http_request.h>
#include <regex> // regular expression 正则表达式

using cppcms::http::response;

UserService::UserService(cppcms::service &srv)
	:BaseService(srv)
{
    dispatcher().map("POST", "", &UserService::register_user, this);
    dispatcher().map("PUT", "", &UserService::modify_user_info, this);

    dispatcher().map("GET", "/get_all_user_list", &UserService::get_user_list, this);
    dispatcher().map("GET", "/get_all_user_list/page/(\\d+)/(\\d+)", &UserService::get_user_list, this, 1, 2);
    dispatcher().map("POST", "/login", &UserService::user_login, this);
    dispatcher().map("PUT", "/change_password", &UserService::modify_user_password, this, 1);
    dispatcher().map("PUT", "/change_password/(.*)", &UserService::modify_user_password, this, 1);

    dispatcher().map("GET", "/(.*)", &UserService::get_user_info, this, 1);
    dispatcher().map("DELETE", "/(.*)", &UserService::delete_user, this, 1);
}

void UserService::user_login()
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

    // 获取用户的登陆信息并验证有效性
    //===============================->
    const std::string& strUserName = request().post("user_name");
    const std::string& strPwd = request().post("user_password");
    if (strUserName.empty() || strPwd.empty())
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request, action(), "用户名或密码不能为空");
        return;
    }

    if (strUserName.length() < 5)
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request, action(), "用户名不小于五位");
        return;
    }
    //<-===============================

    // 加密用户密码
    //===============================->
    const std::string& strPassword = boost::to_upper_copy<std::string>(sha256(md5(strPwd)));
    //<-===============================

    // 验证登陆信息
    //===============================->
    try
    {
        //查询用户信息
        const std::unique_ptr<dbo::Session>& pSession = dbo_session();
        dbo::Transaction transaction(*pSession);
        const dbo::ptr<User>& pUser = pSession->find<User>()
                .where("user_name=?").bind(strUserName)
                .where("user_status=?").bind(UserStatus::ENABLE);
        if (!pUser)
        {
            response().status(response::bad_request);
            response().out() << json_serializer(response::bad_request, action(), "用户不存在, 登陆失败");
            return;
        }

        //比对用户密码
        if (pUser->getUserInfo()->getPassword() != strPassword)
        {
            response().status(response::bad_request);
            response().out() << json_serializer(response::bad_request, action(), "密码错误, 登陆失败");
            return;
        }

        //生成TOKEN
        Token token;
        const std::string& strToken = AuthorizeInstance::Instance().create_token(pUser);
        token.setToken(strToken);
        response().status(response::ok);
        response().out() << json_serializer(token,response::ok, action(), "登陆成功");
        return;
    }
    catch (const std::exception& ex)
    {
        response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error, action(), "服务器内部出错");
        return;
    }
    //<-===============================
}

void UserService::get_user_list()
{
    // 此接口仅超级管理员，首先验证TOKEN
    //===============================->
    const cppcms::string_key& strToken = request().getenv("HTTP_AUTHORIZATION");
    if (strToken.empty())
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request,action(),"认证失败");
        return;
    }
    std::pair<bool, cppcms::string_key> verify = AuthorizeInstance::Instance().verify_token(strToken);
    if (!verify.first)
    {
        response().status(response::unauthorized);
        response().out() << json_serializer(response::unauthorized,action(),"认证失败");
        return;
    }
    //<-===============================

    // 验证用户是不是超级管理员, 不信任Token直接数据库去找
    //===============================->
    try
    {
        const std::string &strUserId = AuthorizeInstance::Instance().get_user_id(strToken);
        const std::unique_ptr<dbo::Session> &pSession = dbo_session();
        dbo::Transaction transaction(*pSession);
        dbo::ptr<User> pUser = pSession->find<User>().where("user_id=?").bind(strUserId);
        if (!pUser)
        {
            response().status(response::bad_request);
            response().out() << json_serializer(response::bad_request, action(), "认证失败");
            return;
        }
        if (pUser->getUserRole() != UserRole::ADMINISTRATOR)
        {
            response().status(response::bad_request);
            response().out() << json_serializer(response::bad_request, action(), "认证失败");
            return;
        }
        //<-===============================

        // 获取所有用户信息并序列化成JSON数据
        //===============================->
        Users users = pSession->find<User>();
        const std::string &strJson = json_serializer(users, 200, action(), "获取用户列表成功");
        //<-===============================

        // 需要将敏感信息删除
        //===============================->
        rapidjson::Document document;
        if (document.Parse(strJson).HasParseError())
        {
            response().status(response::internal_server_error);
            response().out() << json_serializer(response::internal_server_error,action(),"未知错误");
            return;
        }

        if (document.HasMember("data") && document["data"].IsArray())
        {
            rapidjson::Value& data = document["data"];
            for (rapidjson::SizeType i = 0; i < data.Size(); ++i)
            {
                rapidjson::Value& user = data[i];
                if (user.HasMember("user_info") && user["user_info"].IsObject())
                {
                    rapidjson::Value& info = user["user_info"];
                    if (!info.RemoveMember("user_password"))
                    {
                        //如果有一个移除失败都不能返回。这个嵌套太多了，暂时先就这样吧。
                        response().status(response::internal_server_error);
                        response().out() << json_serializer(response::internal_server_error,action(),"系统错误");
                        return;
                    }
                }
            }
        }

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
}

void UserService::get_user_list(unsigned nPageSize, unsigned nPageIndex)
{
    // 此接口仅超级管理员，首先验证TOKEN
    //===============================->
    const cppcms::string_key& strToken = request().getenv("HTTP_AUTHORIZATION");
    if (strToken.empty())
    {
        response().status(response::unauthorized);
        response().out() << json_serializer(response::unauthorized,action(),"认证失败");
        return;
    }
    std::pair<bool, cppcms::string_key> verify = AuthorizeInstance::Instance().verify_token(strToken);
    if (!verify.first)
    {
        response().status(response::unauthorized);
        response().out() << json_serializer(response::unauthorized,action(),"认证失败");
        return;
    }
    //<-===============================

    // 验证用户是不是超级管理员, 不信任Token直接数据库去找
    //===============================->
    try
    {
        const std::string &strUserId = AuthorizeInstance::Instance().get_user_id(strToken);
        const std::unique_ptr<dbo::Session> &pSession = dbo_session();
        dbo::Transaction transaction(*pSession);
        dbo::ptr<User> pUser = pSession->find<User>().where("user_id=?").bind(strUserId);
        if (!pUser)
        {
            response().status(response::bad_request);
            response().out() << json_serializer(response::bad_request, action(), "认证失败");
            return;
        }
        if (pUser->getUserRole() != UserRole::ADMINISTRATOR)
        {
            response().status(response::bad_request);
            response().out() << json_serializer(response::bad_request, action(), "认证失败");
            return;
        }
        //<-===============================

        // 获取所有用户信息并序列化成JSON数据
        //===============================->
        Users users = pSession->find<User>().offset((nPageIndex - 1) * nPageSize).limit(nPageSize);
        const std::string &strJson = json_serializer(users, 200, action(), "获取用户列表成功");
        //<-===============================

        // 需要将敏感信息删除
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
                rapidjson::Value &user = data[i];
                if (user.HasMember("user_info") && user["user_info"].IsObject())
                {
                    rapidjson::Value &info = user["user_info"];
                    if (!info.RemoveMember("user_password"))
                    {
                        //如果有一个移除失败都不能返回。这个嵌套太多了，暂时先就这样吧。
                        response().status(response::internal_server_error);
                        response().out() << json_serializer(response::internal_server_error, action(), "系统错误");
                        return;
                    }
                }
            }
        }

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
}

void UserService::get_user_info(const std::string &strUserId)
{
    // 判断TOKEN合法性
    //===============================->
    const cppcms::string_key& strToken = request().getenv("HTTP_AUTHORIZATION");
    if (strToken.empty())
    {
        response().status(response::unauthorized);
        response().out() << json_serializer(response::unauthorized,action(),"认证失败");
        return;
    }
    std::pair<bool, cppcms::string_key> verify = AuthorizeInstance::Instance().verify_token(strToken);
    if (!verify.first)
    {
        response().status(response::unauthorized);
        response().out() << json_serializer(response::unauthorized,action(),"认证失败");
        return;
    }
    //<-===============================

    // 获取用户信息
    //===============================->
    try
    {
        const std::unique_ptr<dbo::Session> &pSession = dbo_session();
        dbo::Transaction transaction(*pSession);
        const dbo::ptr<User> &pUser = pSession->find<User>().where("user_id=?").bind(strUserId);
        if (!pUser)
        {
            response().status(response::not_found);
            response().out() << json_serializer(response::not_found, action(), "用户不存在");
            return;
        }
        const std::string &strJson = json_serializer(pUser, response::ok, action(), "获取成功");

        if (AuthorizeInstance::Instance().is_admin(strToken))
        {
            response().status(response::ok);
            response().out() << strJson;
            return;
        }
        //<-===============================

        // 去除关键信息才能返回
        //===============================->
        rapidjson::Document document;
        if (document.Parse(strJson).HasParseError())
        {
            response().status(response::internal_server_error);
            response().out() << json_serializer(response::internal_server_error, action(), "未知错误");
            return;
        }

        if (document.HasMember("data") && document["data"].IsObject())
        {
            rapidjson::Value &user = document["data"];
            user.RemoveMember("user_role");
            user.RemoveMember("articles");
            user.RemoveMember("user_status");
            if (user.HasMember("user_info") && user["user_info"].IsObject())
            {
                rapidjson::Value &info = user["user_info"];
                info.RemoveMember("user_ip");
                info.RemoveMember("info_id");
                info.RemoveMember("user_info");
                if (!info.RemoveMember("user_password"))
                {
                    response().status(response::internal_server_error);
                    response().out() << json_serializer(response::internal_server_error, action(), "系统错误");
                    return;
                }
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
}

void UserService::register_user()
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

    // 读取用户请求的消息
    // 注册用户需要用户名、密码、邮箱
    //===============================->
    const std::string& strUserName = request().post("user_name");
    const std::string& strEMail = request().post("user_email");
    const std::string& strPassword = request().post("user_password");
    //<-===============================

    // 验证用户输入合法性
    //===============================->
    if (strUserName.empty() || strPassword.empty() || strEMail.empty())
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request, action(), "用户名密码或邮箱不能为空");
        return;
    }
    if (strUserName.length() < 5)
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request, action(), "用户名长度不小于五");
        return;
    }
    if (strEMail.length() < 5)
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request, action(), "邮箱长度不小于五");
        return;
    }
    const std::regex reg(R"([\w!#$%&'*+/=?^_`{|}~-]+(?:\.[\w!#$%&'*+/=?^_`{|}~-]+)*@(?:[\w](?:[\w-]*[\w])?\.)+[\w](?:[\w-]*[\w])?)");
    if (!std::regex_match(strEMail, reg))
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request, action(), "邮箱格式不正确");
        return;
    }
    if (strPassword.length() < 10)
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request, action(), "密码长度不小于十");
        return;
    }
    const std::regex pwd_reg(R"(^[a-zA-Z]\w{5,17}$)");
    if (!std::regex_match(strPassword, pwd_reg))
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request, action(), "密码必须以字母开头，长度在6~18之间，只能包含字母、数字和下划线");
        return;
    }
    //<-===============================

    try
    {
        // 验证用户名是否重复
        //===============================->
        const std::unique_ptr<dbo::Session>& pSession = dbo_session();
        dbo::Transaction transaction(*pSession);
        const dbo::ptr<User>& pUser = pSession->find<User>().where("user_name=?").bind(strUserName);
        if (pUser)
        {
            response().status(response::bad_request);
            response().out() << json_serializer(response::bad_request, action(), "用户名已存在");
            return;
        }
        //<-===============================

        // 加密用户密码
        //===============================->
        const std::string& strUserPassword = boost::to_upper_copy<std::string>(sha256(md5(strPassword)));
        //<-===============================

        // 创建User实体类和UserInfo实体类
        //===============================->
        const std::string& strAddr = request().remote_addr();
        const std::string& strHost = request().remote_host();

        std::unique_ptr<User> pRegUser = boost::make_unique<User>();
        pRegUser->setUserName(strUserName);
        pRegUser->setUserRole(UserRole::MEMBER);
        pRegUser->setUserStatus(UserStatus::ENABLE);
        pRegUser->setDisplayName(strUserName);

        std::unique_ptr<UserInfo> pRegInfo = boost::make_unique<UserInfo>();
        pRegInfo->setEmail(strEMail);
        pRegInfo->setLevel(0);
        pRegInfo->setIp(strAddr.empty() ? strHost : strAddr);
        pRegInfo->setPassword(strUserPassword);
        //<-===============================

        // 添加到数据库
        //===============================->
        dbo::ptr<User> pAddedUser = pSession->add(std::move(pRegUser));
        if (!pAddedUser)
        {
            response().status(response::bad_request);
            response().out() << json_serializer(response::bad_request, action(), "注册失败, 未知错误");
            return;
        }

        const dbo::ptr<UserInfo> pAddedInfo = pSession->add(std::move(pRegInfo));
        if (!pAddedInfo)
        {
            response().status(response::bad_request);
            response().out() << json_serializer(response::bad_request, action(), "注册失败, 未知错误");
            return;
        }

        pAddedUser.modify()->setUserInfo(pAddedInfo);

        response().status(response::ok);
        response().out() << json_serializer(response::ok, action(), "注册成功");
        return;
        //<-===============================
    }
    catch (const std::exception& ex)
    {
        response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error, action(), "服务器内部出错");
        return;
    }
}

void UserService::delete_user(const std::string &strUserId)
{
    // 此接口仅超级管理员，首先验证TOKEN
    //===============================->
    const cppcms::string_key& strToken = request().getenv("HTTP_AUTHORIZATION");
    if (strToken.empty())
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request,action(),"认证失败");
        return;
    }
    std::pair<bool, cppcms::string_key> verify = AuthorizeInstance::Instance().verify_token(strToken);
    if (!verify.first)
    {
        response().status(response::unauthorized);
        response().out() << json_serializer(response::unauthorized,action(),"认证失败");
        return;
    }
    //<-===============================

    try
    {
        // 验证用户是不是超级管理员, 不信任Token直接数据库去找
        //===============================->
        const std::string &strAdminId = AuthorizeInstance::Instance().get_user_id(strToken);
        const std::unique_ptr<dbo::Session> &pSession = dbo_session();
        dbo::Transaction transaction(*pSession);
        dbo::ptr<User> pUser = pSession->find<User>().where("user_id=?").bind(strUserId);
        if (!pUser)
        {
            response().status(response::bad_request);
            response().out() << json_serializer(response::bad_request, action(), "认证失败");
            return;
        }
        if (pUser->getUserRole() != UserRole::ADMINISTRATOR)
        {
            response().status(response::bad_request);
            response().out() << json_serializer(response::bad_request, action(), "认证失败");
            return;
        }
        //<-===============================

        // 验证超级管理员成功，开始逻辑删除用户
        //===============================->
        pUser.reset(nullptr);
        pUser = pSession->find<User>().where("user_id=?").bind(strUserId);
        if (!pUser)
        {
            response().status(response::not_found);
            response().out() << json_serializer(response::not_found, action(), "未找到该用户");
            return;
        }

        pUser.modify()->setUserStatus(UserStatus::DISABLE);

        response().status(response::ok);
        response().out() << json_serializer(response::ok, action(), "删除成功");
        //<-===============================
    }
    catch (const std::exception& ex)
    {
        response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error, action(), "服务器内部出错");
        return;
    }
}

void UserService::modify_user_info()
{
    // 普通用户可以修改的信息：签名、头像、用户名、(邮箱)
    //===============================->
    const cppcms::string_key& strToken = request().getenv("HTTP_AUTHORIZATION");
    if (strToken.empty())
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request,action(),"认证失败");
        return;
    }
    std::pair<bool, cppcms::string_key> verify = AuthorizeInstance::Instance().verify_token(strToken);
    if (!verify.first)
    {
        response().status(response::unauthorized);
        response().out() << json_serializer(response::unauthorized,action(),"认证失败");
        return;
    }
    //<-===============================

    // 接收用户发送的修改信息
    //===============================->
    try
    {
        const std::unique_ptr<dbo::Session>& pSession = dbo_session();
        dbo::Transaction transaction(*pSession);
        dbo::ptr<User> pUser = dbo::make_ptr<User>();

        char strBuffer[request().raw_post_data().second + 1];
        memcpy(strBuffer, static_cast<char *>(request().raw_post_data().first),
               request().raw_post_data().second);
        json_unserializer(strBuffer, pUser);

        dbo::ptr<User> pModify = pSession->find<User>().where("user_id=?").bind(pUser->getUserId());
        if (!pModify)
        {
            response().status(response::not_found);
            response().out() << json_serializer(response::not_found, action(), "未找到该用户");
            return;
        }

        rapidjson::Document document;
        document.Parse(strBuffer);
        const bool bISAdmin = AuthorizeInstance::Instance().is_admin(strToken);
        if (bISAdmin)
        {

            pModify.modify()->setUserStatus(pUser->getUserStatus());
            pModify.modify()->setUserName(pUser->getUserName());
            pModify.modify()->setDisplayName(pUser->getDisplayName());
            pModify.modify()->setUserRole(pUser->getUserRole());

            if (document.HasMember("user_email") && document.IsString())
            {
                pModify->getUserInfo().modify()->setEmail(document["user_email"].GetString());
            }
            if (document.HasMember("user_signature") && document.IsString())
            {
                pModify->getUserInfo().modify()->setSignature(document["user_signature"].GetString());
            }
            if (document.HasMember("user_profile_photo") && document.IsString())
            {
                pModify->getUserInfo().modify()->setProfilePhoto(document["user_profile_photo"].GetString());
            }
            //pModify->getUserInfo().modify()->setLevel();
        }
        else
        {
            pModify.modify()->setDisplayName(pUser->getDisplayName());

            if (document.HasMember("user_email") && document.IsString())
            {
                pModify->getUserInfo().modify()->setEmail(document["user_email"].GetString());
            }
            if (document.HasMember("user_signature") && document.IsString())
            {
                pModify->getUserInfo().modify()->setSignature(document["user_signature"].GetString());
            }
            if (document.HasMember("user_profile_photo") && document.IsString())
            {
                pModify->getUserInfo().modify()->setProfilePhoto(document["user_profile_photo"].GetString());
            }
        }
    }
    catch (const std::exception& ex)
    {
        response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error, action(), "服务器内部出错");
        return;
    }
    //<-===============================
}

void UserService::modify_user_password(const std::string& strUserId)
{
    std::string strId;

    strId.clear();

    // 验证TOKEN
    //===============================->
    const cppcms::string_key& strToken = request().getenv("HTTP_AUTHORIZATION");
    if (strToken.empty())
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request,action(),"认证失败");
        return;
    }
    std::pair<bool, cppcms::string_key> verify = AuthorizeInstance::Instance().verify_token(strToken);
    if (!verify.first)
    {
        response().status(response::unauthorized);
        response().out() << json_serializer(response::unauthorized,action(),"认证失败");
        return;
    }
    //<-===============================

    // 带参：修改指定用户密码。不带参：修改token所属用户的密码
    //===============================->
    const std::string strTokenUserId = AuthorizeInstance::Instance().get_user_id(strToken);
    // const std::string strId = strUserId.empty() ? strTokenUserId : strUserId;
    const bool bIsAdmin = AuthorizeInstance::Instance().is_admin(strToken);
    if (bIsAdmin)
    {
        strId = strUserId.empty() ? strTokenUserId : strUserId;
    }
    else
    {
        strId = strTokenUserId;
    }
    //<-===============================

    try
    {
        std::unique_ptr<dbo::Session>& pSession = dbo_session();
        dbo::Transaction transaction(*pSession);
        dbo::ptr<User> pUser = pSession->find<User>().where("user_id=?").bind(strId);
        if (!pUser)
        {
            response().status(response::not_found);
            response().out() << json_serializer(response::not_found, action(), "该用户未找到");
            return;
        }

        std::string strPassword = request().post("user_password");
        if (strPassword.empty())
        {
            response().status(response::not_found);
            response().out() << json_serializer(response::not_found, action(), "没有检测到有密码");
            return;
        }
        if (strPassword.length() < 10)
        {
            response().status(response::bad_request);
            response().out() << json_serializer(response::bad_request, action(), "密码长度不小于十");
            return;
        }
        const std::regex pwd_reg(R"(^[a-zA-Z]\w{5,17}$)");
        if (!std::regex_match(strPassword, pwd_reg))
        {
            response().status(response::bad_request);
            response().out() << json_serializer(response::bad_request, action(), "密码必须以字母开头，长度在6~18之间，只能包含字母、数字和下划线");
            return;
        }

        strPassword = boost::to_upper_copy<std::string>(sha256(md5(strPassword)));

        pUser->getUserInfo().modify()->setPassword(strPassword);
        response().status(response::ok);
        response().out() << json_serializer(response::ok, action(), "密码修改成功");
    }
    catch (const std::exception& ex)
    {
        response().status(response::internal_server_error);
        response().out() << json_serializer(response::internal_server_error, action(), "服务器内部出误");
    }
}

