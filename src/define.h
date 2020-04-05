#ifndef _DEFINE_H_
#define _DEFINE_H_

#include "utils/DboInstence.h"

#include <cpr/cpr.h>
#include <plog/Log.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <openssl/md5.h>
#include <openssl/sha.h>

#include <string>
#include <chrono>

class User;
class UserInfo;
class Article;
class Category;
class SystemLog;

namespace dbo = Wt::Dbo;

using Users = dbo::collection<dbo::ptr<User> >;
using UserInfos = dbo::collection<dbo::ptr<UserInfo> >;
using Categorys = dbo::collection<dbo::ptr<Category> >;
using Articles = dbo::collection<dbo::ptr<Article> >;
using Logs = dbo::collection< dbo::ptr<SystemLog> >;

static std::string GenerateUUID()
{
	boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
	std::string uuid_string = boost::uuids::to_string(a_uuid);
	return std::move(boost::to_upper_copy<std::string>(uuid_string));
}

static int startsWith(const std::string& strSrc, const std::string& strSub)
{
    return strSrc.find(strSub) == 0 ? 1 : 0;
}

static int endsWith(const std::string& strSrc, const std::string& strSub)
{
    return strSrc.rfind(strSub) == (strSrc.length()-strSub.length()) ? 1 : 0;
}

// ---- md5摘要哈希 ---- //
static std::string md5(const std::string &srcStr)
{
    // 调用md5哈希
    unsigned char mdStr[33] = {0};
    MD5((const unsigned char *)srcStr.c_str(), srcStr.length(), mdStr);

    // 哈希后的字符串
    //encodedStr = std::string((const char *)mdStr);

    // 哈希后的十六进制串 32字节
    char buf[65] = {0};
    char tmp[3] = {0};
    for (int i = 0; i < 32; i++)
    {
        sprintf(tmp, "%02x", mdStr[i]);
        strcat(buf, tmp);
    }
    buf[32] = '\0'; // 后面都是0，从32字节截断
    return std::string(buf);
}

// ---- sha256摘要哈希 ---- //
static std::string sha256(const std::string &srcStr)
{
    // 调用sha256哈希
    unsigned char mdStr[33] = {0};
    SHA256((const unsigned char *)srcStr.c_str(), srcStr.length(), mdStr);

    // 哈希后的字符串
    //encodedStr = std::string((const char *)mdStr);

    // 哈希后的十六进制串 32字节
    char buf[65] = {0};
    char tmp[3] = {0};
    for (int i = 0; i < 32; i++)
    {
        sprintf(tmp, "%02x", mdStr[i]);
        strcat(buf, tmp);
    }
    buf[32] = '\0'; // 后面都是0，从32字节截断
    return std::string(buf);
}

#endif
