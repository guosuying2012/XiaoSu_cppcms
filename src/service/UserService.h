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

    /**
     * 用户登陆
     */
	void user_login();

	/**
	 * 获取所有用户列表(仅超级管理员使用)
	 */
	void get_user_list();

	/**
	 * 分页获取用户列表(仅超级管理员使用)
	 * @param nPageSize 每页尺寸
	 * @param nPageIndex 当前页码
	 */
	void get_user_list(unsigned nPageSize, unsigned nPageIndex);

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
	 void modify_user_password(const std::string& strUserId = std::string());
};


#endif //XIAOSU_USERSERVICE_H
