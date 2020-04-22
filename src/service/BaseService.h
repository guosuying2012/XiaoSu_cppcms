//
// Created by yengsu on 2020/2/23.
//

#ifndef XIAOSU_BASESERVICE_H
#define XIAOSU_BASESERVICE_H

#include <string>

#include <cppcms/application.h>
#include <cppcms/string_key.h>
#include <jwt-cpp/jwt.h>

#include "define.h"
#include "utils/DboInstence.h"

class BaseService : public cppcms::application
{
public:
    explicit BaseService(cppcms::service& srv);
    ~BaseService() override = default;

    inline cppcms::string_key action() const
    {
        return m_strAction;
	}

	inline std::unique_ptr<Wt::Dbo::Session>& dbo_session() const
	{
        return DboInstance::Instance().Session();
	}

private:
    void main(std::string url) override;

private:
	cppcms::string_key m_strAction;
};

#endif //XIAOSU_BASESERVICE_H
