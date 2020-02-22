//  Created by yengsu on 2020/2/6.

#ifndef _DBOINSTENCE_H
#define _DBOINSTENCE_H

#include <Wt/Dbo/Dbo.h>
#include <cppcms/json.h>
#include <Wt/Dbo/Session.h>
#include <boost/make_unique.hpp>
#include <Wt/Dbo/backend/MySQL.h>
#include <Wt/Dbo/FixedSqlConnectionPool.h>

class DboSingleton
{
public:
	static DboSingleton& GetInstance(const cppcms::json::value& setting = cppcms::json::value())
	{
		static DboSingleton dboSingleton(setting);
		return dboSingleton;
	}

	std::unique_ptr<Wt::Dbo::Session>& GetSession()
	{
		return m_pSession;
	}

public:
	~DboSingleton() = default;
	DboSingleton(DboSingleton const&) = delete;
	DboSingleton(DboSingleton const&&) = delete;
	DboSingleton& operator=(DboSingleton const&) = delete;
	DboSingleton& operator=(DboSingleton const&&) = delete;

private:
	explicit DboSingleton(const cppcms::json::value& setting)
	{
		const std::string& strHost = setting.get<std::string>("database.host");
		const std::string& strDBName = setting.get<std::string>("database.dbname");
		const std::string& strUser = setting.get<std::string>("database.user");
		const std::string& strPsw = setting.get<std::string>("database.password");
		const unsigned& unPort = setting.get<int>("database.port");
		const int& nPoolSize = setting.get<int>("database.pool_size");

		m_pMySql = boost::make_unique<Wt::Dbo::backend::MySQL>(strDBName, strUser, strPsw, strHost, unPort);
		m_pConnPool = boost::make_unique<Wt::Dbo::FixedSqlConnectionPool>(std::move(m_pMySql), nPoolSize);
		m_pSession = boost::make_unique<Wt::Dbo::Session>();
		m_pSession->setConnectionPool(*m_pConnPool);
	}

private:
	std::unique_ptr<Wt::Dbo::Session> m_pSession;
	std::unique_ptr<Wt::Dbo::backend::MySQL> m_pMySql;
	std::unique_ptr<Wt::Dbo::SqlConnectionPool> m_pConnPool;
};

#endif //HELLOCLION_DBOINSTENCE_H
