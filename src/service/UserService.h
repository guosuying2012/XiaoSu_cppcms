//
// Created by yengsu on 2020/4/21.
//

#ifndef XIAOSU_USERSERVICE_H
#define XIAOSU_USERSERVICE_H

#include "BaseService.h"

class UserService : public BaseService
{
public:
	explicit UserService(cppcms::service& srv);
	~UserService() override = default;

	//test
	void get_token();

    /**
     * 用户登陆
     */
	void user_login();

	/**
	 * 获取用户信息
	 * @param strUserId 用户ID
	 */
	void get_user_info(const std::string& strUserId);

	/**
	 * 用户注册
	 */
	void register_user();

	/**
	 * 逻辑删除用户
	 * @param strUserId 用户ID
	 */
	void delete_user(const std::string& strUserId);

	/**
	 * 修改用户信息
	 */
	void modify_user_info();

	/**
	 * 修改密码
	 */
	 void modify_user_password();
};


#endif //XIAOSU_USERSERVICE_H
