//
// Created by yengsu on 2020/2/23.
//

#ifndef XIAOSU_BASESERVICE_H
#define XIAOSU_BASESERVICE_H

#include <string>
#include <cppcms/application.h>

class BaseService : public cppcms::application
{
public:
    explicit BaseService(cppcms::service& srv);
    ~BaseService() override = default;
    void main(std::string url) override;
};


#endif //XIAOSU_BASESERVICE_H
