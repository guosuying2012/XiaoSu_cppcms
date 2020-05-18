
//
// Created by yengsu on 2020/4/21.
//
#include "UserService.h"
#include "model/user.h"
#include "model/userinfo.h"
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
    //test
    dispatcher().map("GET", "/get_token", &UserService::get_token, this);

    dispatcher().map("POST", "", &UserService::register_user, this);
    dispatcher().map("PUT", "", &UserService::modify_user_info, this);

    dispatcher().map("GET", "/(.*)", &UserService::get_user_info, this, 1);
    dispatcher().map("DELETE", "/(.*)", &UserService::delete_user, this, 1);

    dispatcher().map("POST", "/login", &UserService::user_login, this);
    dispatcher().map("PUT", "/change_password", &UserService::modify_user_password, this);
}

void UserService::get_token()
{
    const std::unique_ptr<dbo::Session>& pSession = dbo_session();
    dbo::Transaction transaction(*pSession);

    dbo::ptr<User> pUser = pSession->find<User>()
            .where("user_id=?")
            .bind("6101d51a-c54b-4594-acf3-25ee1c1374bd");
    response().out() << AuthorizeInstance::Instance().create_token(pUser);
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
        const dbo::ptr<User>& pUser = pSession->find<User>().where("user_name=?").bind(strUserName);
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

void UserService::get_user_info(const std::string &strUserId)
{

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
    if (strPassword.length() < 10)
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request, action(), "密码长度不小于十");
        return;
    }
    const std::regex reg(R"([\w!#$%&'*+/=?^_`{|}~-]+(?:\.[\w!#$%&'*+/=?^_`{|}~-]+)*@(?:[\w](?:[\w-]*[\w])?\.)+[\w](?:[\w-]*[\w])?)");
    if (!std::regex_match(strEMail, reg))
    {
        response().status(response::bad_request);
        response().out() << json_serializer(response::bad_request, action(), "邮箱格式不正确");
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
        pRegUser->setUserStatus(UserStatus::DISABLE);
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

}

void UserService::modify_user_info()
{

}

void UserService::modify_user_password()
{

}

