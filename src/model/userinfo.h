//
// Created by yengsu on 2020/2/29.
//

#ifndef XIAOSU_USERINFO_H
#define XIAOSU_USERINFO_H

#include "define.h"

class UserInfo
{
public:
	UserInfo()
        :m_nLevel(0), m_strId(GenerateUUID()),
         m_unRegTime(std::chrono::system_clock::now())
	{
		m_strIp.clear();
		m_strEmail.clear();
		m_strSignature.clear();
		m_strProfilePhoto.clear();
	}

public:
	template<class Action>
	void persist(Action& a)
    {
        dbo::id(a, m_strId, "info_id", 36);
		dbo::field(a, m_strIp, "user_ip", 20);
        dbo::belongsTo(a, m_pUser, "info_id");
		dbo::field(a, m_strEmail, "user_email", 50);
		dbo::field(a, m_strPassword, "user_password");
        dbo::field(a, m_unRegTime, "user_registration");
		dbo::field(a, m_strSignature, "user_signature", 120);
		dbo::field(a, m_strProfilePhoto, "user_profile_photo");
	}

private:
	int m_nLevel;
    std::string m_strId;
    std::string m_strIp;
	dbo::ptr<User> m_pUser;
	std::string m_strEmail;
	std::string m_strPassword;
	std::string m_strSignature;
	std::string m_strProfilePhoto;
	std::chrono::system_clock::time_point m_unRegTime;
};

#endif //XIAOSU_USERINFO_H
