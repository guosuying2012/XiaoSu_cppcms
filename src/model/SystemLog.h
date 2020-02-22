//
// Created by yengsu on 2020/1/23.
//

#ifndef HELLOCLION_SYS_LOG_H
#define HELLOCLION_SYS_LOG_H

#include "../define.h"

class SystemLog
{
public:
    SystemLog() :
	m_strId(GenerateUUID()),
	m_unTid(0),
	m_nLine(0),
	m_severity(plog::Severity::none)
	{
	}

	const std::string &getId() const
	{
		return this->m_strId;
	}

	inline const std::chrono::system_clock::time_point &getTime() const
	{
		return this->m_unTime;
	}

	inline unsigned long getTimeStamp() const
	{
		auto tp = std::chrono::time_point_cast<std::chrono::milliseconds>(this->m_unTime);
		std::time_t timestamp =  tp.time_since_epoch().count();
		return timestamp;
	}

	inline int getLine() const
	{
		return this->m_nLine;
	}

	inline const std::string &getFunName() const
	{
		return this->m_strFunName;
	}

	inline const plog::Severity &getSeverity() const
	{
		return this->m_severity;
	}

	inline long getTid() const
	{
		return this->m_unTid;
	}

	inline const std::string &getMessage() const
	{
		return this->m_strMessage;
	}

	inline void setTime(std::chrono::system_clock::time_point unTime)
	{
        this->m_unTime = unTime;
	}

	inline void setLine(int nLine)
	{
        this->m_nLine = nLine;
	}

	inline void setFunName(const std::string &strFunName)
	{
        this->m_strFunName = strFunName;
	}

	inline void setSeverity(const plog::Severity &severity)
	{
        this->m_severity = severity;
        this->m_strSeverity = severityToString(severity);
	}

	inline void setTid(long nTid)
	{
        this->m_unTid = nTid;
	}

	inline void setMessage(const std::string &strMessage)
	{
		this->m_strMessage = strMessage;
	}

public:
    template<class Action>
    void persist(Action& a)
    {
        dbo::id(a, m_strId, "log_id", 36);
	    dbo::field(a, m_unTid, "tid");
	    dbo::field(a, m_nLine, "line");
	    dbo::field(a, m_unTime, "time");
	    dbo::field(a, m_strMessage, "message");
	    dbo::field(a, m_strFunName, "function");
	    dbo::field(a, m_severity, "serverity");
	    dbo::field(a, m_strSeverity, "str_serverity");
    }

public:
	int m_nLine;
	long m_unTid;
	std::string m_strId;
	std::string m_strFunName;
	std::string m_strMessage;
	std::string m_strSeverity;
	plog::Severity m_severity;
	std::chrono::system_clock::time_point m_unTime;
};

DISENABLE_DEFAULT_KEY(SystemLog);

#endif //HELLOCLION_SYS_LOG_H
