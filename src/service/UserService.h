//
// Created by yengsu on 2020/4/21.
//

#ifndef XIAOSU_USERSERVICE_H
#define XIAOSU_USERSERVICE_H

#include "BaseService.h"

class UserService : public BaseService
{
public:
	explicit UserService(cppcms::service& srv);
	~UserService() override = default;
};


#endif //XIAOSU_USERSERVICE_H
