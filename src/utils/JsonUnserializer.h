//
// Created by yengsu on 2020/3/9.
//

#ifndef XIAOSU_JSONUNSERIALIZER_H
#define XIAOSU_JSONUNSERIALIZER_H

#include "define.h"
#include "DboInstence.h"

#include <Wt/Dbo/ptr.h>
#include <Wt/Dbo/Field.h>
#include <Wt/Dbo/weak_ptr.h>
#include <Wt/Dbo/collection.h>

#include <rapidjson/document.h>

#include <chrono>
#include <iomanip>
#include <type_traits>

class JsonUnserializer
{
public:
	JsonUnserializer() = default;
	virtual ~JsonUnserializer() = default;
	JsonUnserializer(const JsonUnserializer&) = delete;
	void operator=(const JsonUnserializer&) = delete;
	JsonUnserializer(const JsonUnserializer&&) = delete;
	void operator=(const JsonUnserializer&&) = delete;

	const Wt::Dbo::Session *session() const { return DboSingleton::GetInstance().GetSession().get(); }

	template<typename T>
    void unserialize(const std::string& strJson, Wt::Dbo::ptr<T>& t)
	{
		if (m_Document.Parse(strJson).HasParseError())
		{
			return;
		}

		if (!t)
		{
            t = Wt::Dbo::make_ptr<T>();
		}
		const_cast<T&>(*t).persist(*this);
	}

	//枚举类型
	template<typename T>
	typename std::enable_if< !std::is_enum<T>::value, void>::type
	act(Wt::Dbo::FieldRef<T> field);
	template<typename T>
	typename std::enable_if< std::is_enum<T>::value, void>::type
	act(Wt::Dbo::FieldRef<T> field)
	{
		//枚举类型
		if(m_Document.HasMember(field.name()) && m_Document[field.name()].IsInt())
		{
			field.setValue((T)m_Document[field.name()].GetInt());
		}
	}

	//各种类型
	void act(const Wt::Dbo::FieldRef<std::string>& field)
	{
		if (m_Document.HasMember(field.name()) && m_Document[field.name()].IsString())
		{
            if (endsWith(field.name(), "_id"))
            {
                return;
            }
			field.setValue(m_Document[field.name()].GetString());
		}
	}
	void act(const Wt::Dbo::FieldRef<long long>& field)
	{
		if (m_Document.HasMember(field.name()) && m_Document[field.name()].IsNumber())
		{
			field.setValue(m_Document[field.name()].GetInt64());
		}
	}
	void act(const Wt::Dbo::FieldRef<int>& field)
	{
		if (m_Document.HasMember(field.name()) && m_Document[field.name()].IsInt())
		{
			field.setValue(m_Document[field.name()].GetInt());
		}
	}
	void act(const Wt::Dbo::FieldRef<long>& field)
	{
		if (m_Document.HasMember(field.name()) && m_Document[field.name()].IsNumber())
		{
			field.setValue(m_Document[field.name()].GetInt64());
		}
	}
	void act(const Wt::Dbo::FieldRef<short>& field)
	{
		if (m_Document.HasMember(field.name()) && m_Document[field.name()].IsNumber())
		{
			field.setValue(m_Document[field.name()].GetInt());
		}
	}
    void act(const Wt::Dbo::FieldRef<bool>& field)  //一般表示状态
	{
        //if (m_Document.HasMember(field.name()) && m_Document[field.name()].IsBool())
		{
            //field.setValue(m_Document[field.name()].GetBool());
		}
	}
	void act(const Wt::Dbo::FieldRef<float>& field)
	{
		if (m_Document.HasMember(field.name()) && m_Document[field.name()].IsFloat())
		{
			field.setValue(m_Document[field.name()].GetFloat());
		}
	}
	void act(const Wt::Dbo::FieldRef<double>& field)
	{
		if (m_Document.HasMember(field.name()) && m_Document[field.name()].IsDouble())
		{
			field.setValue(m_Document[field.name()].GetDouble());
		}
	}
    void act(const Wt::Dbo::FieldRef<std::chrono::system_clock::time_point>& field)//一般表示创建时间或操作时间
	{
        //if (m_Document.HasMember(field.name()) && m_Document[field.name()].IsNumber())
        {
            //const uint64_t& nTime = m_Document[field.name()].GetUint64();
            //field.setValue(std::chrono::system_clock::from_time_t(nTime));
        }
	}
	void act(const Wt::Dbo::FieldRef<std::chrono::duration<int, std::milli> >& field)
	{
		//表示时间段
	}

	template<typename T>
	inline void actId(T& value, const std::string& name, int size)
	{
		Wt::Dbo::field(*this, value, name, size);
	}

	template<typename T>
	inline void actId(Wt::Dbo::ptr<T>& value, const std::string& name, int size, int fkConstraints)
	{
		Wt::Dbo::field(*this, value, name, size);
	}

	template<typename T>
	void actPtr(const Wt::Dbo::PtrRef<T>& field)
    {
        std::string& strFieldName = const_cast<std::string&>(field.name());
        if (m_Document.HasMember(strFieldName) && m_Document[strFieldName].IsString())
        {
            try
            {
                const std::string& strID = m_Document[strFieldName].GetString();
                const std::unique_ptr<dbo::Session>& pSession = DboSingleton::GetInstance().GetSession();
                dbo::Transaction transaction(*pSession);

                Wt::Dbo::ptr<T> pObj = DboSingleton::GetInstance().GetSession()->find<T>()
                        .where(strFieldName.append("=?"))
                        .bind(strID);

                if (pObj)
                {
                    field.value() = boost::make_unique<T>();
                    field.value() = pObj;
                }
                else
                {
                    field.value() = nullptr;
                }
            }
            catch (const std::exception& ex)
            {
                PLOG_ERROR << ex.what();
            }
        }
	}

	template<typename T>
	void actWeakPtr(const Wt::Dbo::WeakPtrRef<T>& field)
    {
        /*std::string& strFieldName = const_cast<std::string&>(field.joinName());
        if (m_Document.HasMember(strFieldName) && m_Document[strFieldName].IsString())
        {
            try
            {
                const std::string& strID = m_Document[strFieldName].GetString();
                const std::unique_ptr<dbo::Session>& pSession = DboSingleton::GetInstance().GetSession();
                dbo::Transaction transaction(*pSession);

                Wt::Dbo::ptr<T> pObj = DboSingleton::GetInstance().GetSession()->find<T>()
                        .where(strFieldName.append("=?"))
                        .bind(strID);

                if (pObj)
                {
                    field.value() = boost::make_unique<T>();
                }
                else
                {
                    field.value() = nullptr;
                }
            }
            catch (const std::exception& ex)
            {
                PLOG_ERROR << ex.what();
            }
        }*/
	}

	template<typename T>
	void actCollection(const Wt::Dbo::CollectionRef<T>& collec)
    {
	}

private:
	rapidjson::Document m_Document;
};

template<typename C>
void json_unserializer(const std::string& strJson, C& pObject)
{
    JsonUnserializer serializer;
    serializer.unserialize(strJson, pObject);
}

#endif //XIAOSU_JSONUNSERIALIZER_H
