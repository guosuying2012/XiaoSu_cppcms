//
// Created by yengsu on 2020/2/29.
//

#ifndef XIAOSU_USER_H
#define XIAOSU_USER_H

#include "define.h"

enum class UserStatus
{
	ENABLE = 0,
	DISABLE
};

enum class UserRole
{
	GUEST = 0,
	GENERAL,
	MEMBER,
	ADMINISTRATOR = 750512656,
};

class User
{
public:
	User()
		:m_strId(GenerateUUID()),
		m_nRole(UserRole::GENERAL),
		m_nStatus(UserStatus::ENABLE)
	{
		m_strName.clear();
	}

	inline Articles getArticles() const
	{
		return this->m_vecArticles;
	}

public:
	template<class Action>
	void persist(Action& a)
	{
		dbo::id(a, m_strId, "user_id", 36);
		dbo::field(a, m_nRole, "user_role", 2);
        dbo::hasOne(a, m_pUserInfo, "info_id");
        dbo::field(a, m_strName, "user_name", 15);
		dbo::field(a, m_nStatus, "user_status", 2);
        dbo::field(a, m_strDisplayName, "user_display_name", 20);
        dbo::hasMany(a, m_vecArticles, dbo::ManyToOne, "user_id");
	}

private:
	UserRole m_nRole;
	std::string m_strId;
    UserStatus m_nStatus;
	std::string m_strName;
	Articles m_vecArticles;
	std::string m_strDisplayName;
    dbo::weak_ptr<UserInfo> m_pUserInfo;
};

#endif //XIAOSU_USER_H
