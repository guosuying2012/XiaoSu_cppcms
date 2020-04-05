//
// Created by yengsu on 2020/2/3.
//

#ifndef _DBOAPPENDER_H
#define _DBOAPPENDER_H

#include "model/systemlog.h"
#include <plog/Appenders/IAppender.h>

class DboAppender : public plog::IAppender
{
public:
	explicit DboAppender() = default;

	void write(const plog::Record& record) override
	{
		plog::util::MutexLock lock(m_mutex);
		try
		{
            dbo::Transaction transaction(*DboInstance::Instance().Session());
			std::unique_ptr<SystemLog> sysLog{new SystemLog()};
			sysLog->setMessage(record.getMessage());
			sysLog->setSeverity(record.getSeverity());
			sysLog->setTid(record.getTid());
			sysLog->setTime(std::chrono::system_clock::now());
			sysLog->setFunName(record.getFunc());
			sysLog->setLine(record.getLine());
            DboInstance::Instance().Session()->add(std::move(sysLog));
			transaction.commit();
		}
		catch (const std::exception &ex)
		{
			PLOG_INFO << "Save Failed: " << record.getMessage() <<
			". FL.[" << record.getFunc() << "]@" << record.getLine() <<
			". msg: " << ex.what();
		}
	}

private:
	plog::util::Mutex m_mutex;
};

#endif //HELLOCLION_DBOAPPENDER_H
