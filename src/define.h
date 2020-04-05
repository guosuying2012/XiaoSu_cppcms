#ifndef _DEFINE_H_
#define _DEFINE_H_

#include "utils/DboInstence.h"

#include <cpr/cpr.h>
#include <plog/Log.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid_generators.hpp>

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

#endif
