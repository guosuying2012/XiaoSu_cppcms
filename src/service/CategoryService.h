#ifndef CATEGORYSERVICE_H
#define CATEGORYSERVICE_H

#include "BaseService.h"

class CategoryService : public BaseService
{
public:
    CategoryService(cppcms::service& srv);
    ~CategoryService() override = default;
};

#endif // CATEGORYSERVICE_H
