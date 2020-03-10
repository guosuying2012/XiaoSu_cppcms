//
// Created by yengsu on 2020/2/29.
//

#ifndef XIAOSU_CATEGORY_H
#define XIAOSU_CATEGORY_H

#include "../define.h"

class Category
{
public:
	Category()
		:m_strId(GenerateUUID()), m_nRank(0)
	{
		m_strParentId.clear();
		m_strCategoryName.clear();
	}

public:
	template<class Action>
	void persist(Action& a)
	{
		dbo::id(a, m_strId, "category_id", 36);
		dbo::field(a, m_strParentId, "category_parent", 36);
		dbo::field(a, m_strCategoryName, "category_name");
		dbo::field(a, m_nRank, "category_rank");
		dbo::hasMany(a, m_vecArticles, dbo::ManyToOne, "category_id");
	}

	inline Articles getArticles() const
	{
		return this->m_vecArticles;
	}

private:
	int m_nRank;
	std::string m_strId;
	Articles m_vecArticles;
	std::string m_strParentId;
	std::string m_strCategoryName;
};

#endif //XIAOSU_CATEGORY_H
