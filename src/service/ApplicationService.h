//
// Created by yengsu on 2020/1/23.
//

#ifndef _APPLICATIONSERVICE_H
#define _APPLICATIONSERVICE_H

#include <string>
#include <cppcms/application.h>

class ApplicationService : public cppcms::application
{
public:
    explicit ApplicationService(cppcms::service& srv);
    ~ApplicationService() override = default;
    void main(std::string url) override;
};

#endif //HELLOCLION_APPLICATIONSERVICE_H
