//
// Created by yengsu on 2020/2/23.
//

#ifndef XIAOSU_BASESERVICE_H
#define XIAOSU_BASESERVICE_H

#include "utils/DboInstence.h"
#include <string>
#include <cppcms/application.h>
#include <cppcms/string_key.h>

class BaseService : public cppcms::application
{
public:
    explicit BaseService(cppcms::service& srv);
    virtual ~BaseService() override = default;

    void main(std::string url) override;

	inline cppcms::string_key action() const
	{
		return this->m_strAction;
	}

	inline std::unique_ptr<Wt::Dbo::Session>& dbo_session() const
	{
        return DboInstance::Instance().Session();
	}

private:
	cppcms::string_key m_strAction;
};


#endif //XIAOSU_BASESERVICE_H
