//
// Created by yengsu on 2020/3/1.
//

#ifndef XIAOSU_ARTICLESERVICE_H
#define XIAOSU_ARTICLESERVICE_H

#include "BaseService.h"

class ArticleService : public BaseService
{
public:
	explicit ArticleService(cppcms::service& srv);
	~ArticleService() override = default;

private:
    void add_article();

    void move_to(const std::string strArticleId, const std::string& strMoveTo, const std::string strId);

    void delete_article(const std::string& strId);

	void article(const std::string& strArticleId);

    void all_articles();
    void all_articles(int nPageSize, int nCurrentPage);

    void all_article_by_user(const std::string& strUserId);
    void all_article_by_user(const std::string& strUserId, int nPageSize, int nCurrentPage);

    void all_article_by_category(const std::string& strCategoryId);
    void all_article_by_category(const std::string& strCategoryId, int nPageSize, int nCurrentPage);
};


#endif //XIAOSU_ARTICLESERVICE_H
