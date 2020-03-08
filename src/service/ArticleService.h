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
	void article(const std::string& strArticleId);

	void allArticles();
	void allArticles(int nPageSize, int nCurrentPage);

	void allArticleByUser(const std::string& strUserId);
	void allArticleByUser(const std::string& strUserId, int nPageSize, int nCurrentPage);

	void allArticleByCategory(const std::string& strCategoryId);
	void allArticleByCategory(const std::string& strCategoryId, int nPageSize, int nCurrentPage);
};


#endif //XIAOSU_ARTICLESERVICE_H
