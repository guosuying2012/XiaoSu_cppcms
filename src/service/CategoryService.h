#ifndef CATEGORYSERVICE_H
#define CATEGORYSERVICE_H

#include "BaseService.h"

class CategoryService : public BaseService
{
public:
    CategoryService(cppcms::service& srv);
    ~CategoryService() override = default;

    void add_category();

    void delete_category(const std::string& strId);

    void modify_category(const std::string& strId);

    void category_list();
};

#endif // CATEGORYSERVICE_H
