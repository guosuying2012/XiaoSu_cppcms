//
// Created by yengsu on 2020/1/23.
//

#ifndef _APPLICATIONSERVICE_H
#define _APPLICATIONSERVICE_H

#include "BaseService.h"

class ApplicationService : public BaseService
{
public:
    explicit ApplicationService(cppcms::service& srv);
    ~ApplicationService() override = default;
};

#endif //_APPLICATIONSERVICE_H
