//
// Created by yengsu on 2020/2/29.
//

#ifndef XIAOSU_USER_H
#define XIAOSU_USER_H

#include "define.h"

enum class UserStatus
{
	ENABLE = 0,
	DISABLE,

	EMAIL_VERIFY,  //等待邮箱验证
};

enum class UserRole
{
	GUEST = 0,                  //游客
	GENERAL,                    //普通用户
	MEMBER,                     //会员
	ADMINISTRATOR = 750512656,  //超级用户
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

	inline void setUserName(const std::string& strUserName)
    {
	    this->m_strName = strUserName;
    }

    inline void setUserStatus(const UserStatus& status)
    {
	    this->m_nStatus = status;
    }

    inline void setDisplayName(const std::string& strDisplayName)
    {
	    this->m_strDisplayName = strDisplayName;
    }

	inline Articles getArticles() const
	{
		return this->m_vecArticles;
	}

	inline UserRole getUserRole() const
	{
	    return this->m_nRole;
	}

	inline void setUserRole(const UserRole& role)
    {
	    this->m_nRole = role;
    }

	inline dbo::weak_ptr<UserInfo> getUserInfo() const
    {
	    return this->m_pUserInfo;
    }

    inline void setUserInfo(const dbo::ptr<UserInfo>& pInfo)
    {
	    this->m_pUserInfo = pInfo;
    }

public:
	template<class Action>
	void persist(Action& a)
	{
		dbo::id(a, m_strId, "user_id", 36);
		dbo::field(a, m_nRole, "user_role", 2);
        dbo::hasOne(a, m_pUserInfo, "user_info");
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

class Token
{
public:
    inline void setToken(const std::string& strToken)
    {
        this->m_strToken = strToken;
    }

    template<class Action>
    void persist(Action& a)
    {
        dbo::field(a, m_strToken, "token");
    }

private:
    std::string m_strToken;
};

#endif //XIAOSU_USER_H
