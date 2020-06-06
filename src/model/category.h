//
// Created by yengsu on 2020/2/29.
//

#ifndef XIAOSU_CATEGORY_H
#define XIAOSU_CATEGORY_H

#include <utility>
#include "define.h"

class Category
{
public:
	Category()
		:m_strId(GenerateUUID()), m_nRank(0)
	{
		m_strParentId.clear();
		m_strCategoryName.clear();
	}

	inline std::string id() const
	{
	    return m_strId;
	}

	inline void set_parent_id(const std::string& strId)
    {
	    this->m_strParentId = strId;
    }

	inline std::string get_parent_id() const
    {
        return this->m_strParentId;
    }

    inline void set_name(const std::string& strName)
    {
	    this->m_strCategoryName = strName;
    }

    inline std::string get_name() const
    {
        return this->m_strCategoryName;
    }

    inline void rank(int nRank)
    {
	    this->m_nRank = nRank;
    }

    inline int rank() const
    {
        return m_nRank;
    }

    inline Articles getArticles() const
    {
        return this->m_vecArticles;
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

private:
	int m_nRank;
	std::string m_strId;
	Articles m_vecArticles;
	std::string m_strParentId;
	std::string m_strCategoryName;
};

#endif //XIAOSU_CATEGORY_H
