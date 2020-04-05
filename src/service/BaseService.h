//
// Created by yengsu on 2020/2/23.
//

#ifndef XIAOSU_BASESERVICE_H
#define XIAOSU_BASESERVICE_H

#include "define.h"
#include "utils/DboInstence.h"

#include <cppcms/application.h>
#include <cppcms/string_key.h>
#include <jwt-cpp/jwt.h>

#include <string>

class BaseService : public cppcms::application
{
public:
    explicit BaseService(cppcms::service& srv);
    virtual ~BaseService() override = default;

    void main(std::string url) override;

    inline cppcms::string_key action() const
    {
        return m_strAction;
	}

	inline std::unique_ptr<Wt::Dbo::Session>& dbo_session() const
	{
        return DboInstance::Instance().Session();
	}

    inline std::string authorization_issuer()
    {
        const std::string& strIssuer = settings().get<std::string>("authorization.issuer");
        return md5(jwt::base::encode<jwt::alphabet::base64url>(strIssuer));
    }

    inline std::string authorization_secret()
    {
        const std::string& strIssuer = settings().get<std::string>("authorization.secret");
        return sha256(jwt::base::encode<jwt::alphabet::base64url>(strIssuer));
    }

private:
	cppcms::string_key m_strAction;
};

#endif //XIAOSU_BASESERVICE_H
