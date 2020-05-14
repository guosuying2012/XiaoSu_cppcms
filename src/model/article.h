//
// Created by yengsu on 2020/2/29.
//

#ifndef XIAOSU_ARTICLE_H
#define XIAOSU_ARTICLE_H

#include "define.h"

class Article
{
public:
	Article()
		:m_strId(GenerateUUID()), m_bApprovalStatus(false)
		, m_unCreateTime(std::chrono::system_clock::now())
		, m_unLastChange(std::chrono::system_clock::now())
	{
		m_strTitle.clear();
		m_strCover.clear();
		m_strContent.clear();
		m_strDescribe.clear();
	}

    inline dbo::ptr<User> user() const
    {
        return m_pUser;
    }

    inline void user(const dbo::ptr<User> &pUser)
    {
        m_pUser = pUser;
    }

    inline dbo::ptr<Category> category() const
    {
        return m_pCategory;
    }

    inline void category(const dbo::ptr<Category> &pCategory)
    {
        m_pCategory = pCategory;
    }

    inline std::string title() const
    {
        return m_strTitle;
    }

    inline void title(const std::string &strTitle)
    {
        m_strTitle = strTitle;
    }

    inline std::string cover() const
    {
        return m_strCover;
    }

    inline void cover(const std::string &strCover)
    {
        m_strCover = strCover;
    }

    inline std::string content() const
    {
        return m_strContent;
    }

    inline void content(const std::string &strContent)
    {
        m_strContent = strContent;
    }

    inline std::string describe() const
    {
        return m_strDescribe;
    }

    inline void describe(const std::string &strDescribe)
    {
        m_strDescribe = strDescribe;
    }

    inline bool approval() const
    {
	    return m_bApprovalStatus;
    }

    inline void approval(const bool bApproval)
    {
	    m_bApprovalStatus = bApproval;
    }

    inline void update_last_change()
    {
	    m_unLastChange = std::chrono::system_clock::now();
    }

public:
	template<class Action>
	void persist(Action& a)
	{
		dbo::id(a, m_strId, "article_id", 36);
		dbo::belongsTo(a, m_pUser, "user_id");
		dbo::belongsTo(a, m_pCategory, "category_id");
		dbo::field(a, m_strTitle, "article_title", 30);
		dbo::field(a, m_strCover, "article_cover");
		dbo::field(a, m_strDescribe, "article_describe", 255);
		dbo::field(a, m_strContent, "article_content");
		dbo::field(a, m_unCreateTime, "article_time");
		dbo::field(a, m_unLastChange, "article_last_change");
		dbo::field(a, m_bApprovalStatus, "article_approval_status", 1);
    }

private:
    std::string m_strId;
    bool m_bApprovalStatus;
    std::string m_strTitle;
    std::string m_strCover;
    dbo::ptr<User> m_pUser;
    std::string m_strContent;
	std::string m_strDescribe;
	dbo::ptr<Category> m_pCategory;
	std::chrono::system_clock::time_point m_unCreateTime;
	std::chrono::system_clock::time_point m_unLastChange;
};

#endif //XIAOSU_ARTICLE_H
