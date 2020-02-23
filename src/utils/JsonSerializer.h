//
// Created by yengsu on 2020/2/20.
//

#ifndef _JSONSERIALIZER_H
#define _JSONSERIALIZER_H

#include <Wt/Dbo/ptr.h>
#include <Wt/Dbo/Field.h>
#include <Wt/Dbo/weak_ptr.h>
#include <Wt/Dbo/collection.h>

#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <chrono>
#include <iomanip>
#include <type_traits>

class JsonSerializer
{
public:
    JsonSerializer(int nStatus, const std::string& strAction, const std::string& strMsg)
    :m_writer(m_buffer), m_pSession(nullptr)
    {
        m_writer.StartObject();
        m_writer.Key("action");
        m_writer.String(strAction);
        m_writer.Key("msg");
        m_writer.String(strMsg);
        m_writer.Key("status");
        m_writer.Int(nStatus);
        m_writer.Key("data");
    }
    virtual ~JsonSerializer() = default;

    JsonSerializer(const JsonSerializer&) = delete;
    void operator=(const JsonSerializer&) = delete;
    JsonSerializer(const JsonSerializer&&) = delete;
    void operator=(const JsonSerializer&&) = delete;

    const Wt::Dbo::Session *session() const { return m_pSession; }

    inline std::string GetString()
    {
        std::string strResponce;
        strResponce.clear();

        m_writer.EndObject();
        strResponce = m_buffer.GetString();

        m_buffer.Flush();
        m_writer.Flush();
        m_buffer.Clear();
        return strResponce;
    }

    void serialize()
    {
        m_writer.StartObject();
        m_writer.EndObject();
    }

    //序列化对象
    template<typename T>
    void serialize(const T& t)
    {
        m_writer.StartObject();
        const_cast<T&>(t).persist(*this);
        m_writer.EndObject();
    }

    //序列化对象指针
    template<typename T>
    void serialize(const Wt::Dbo::ptr<T>& t)
    {
        m_pSession = t.session();
        m_writer.StartObject();
        //如果有默认ID
        if (Wt::Dbo::dbo_traits<T>::surrogateIdField())
        {
            std::stringstream ss;
            ss << t.id();

            m_writer.Key(Wt::Dbo::dbo_traits<T>::surrogateIdField());
            m_writer.String(ss.str());
        }
        const_cast<T&>(*t).persist(*this);
        m_writer.EndObject();
    }

    //序列化集合
    template<typename T>
    void serialize(const std::vector<Wt::Dbo::ptr<T> >& v)
    {
        m_writer.StartArray();
        for (typename std::vector<Wt::Dbo::ptr<T> >::const_iterator i = v.begin(); i != v.end(); ++i)
        {
            if (i == v.begin())
            {
                m_pSession = (*i).session();
            }
            serialize(*i);
        }
        m_writer.EndArray();
    }

    //序列化查询结果
    template<typename T>
    void serialize(const Wt::Dbo::collection<Wt::Dbo::ptr<T> >& c)
    {
        m_pSession = c.session();
        m_writer.StartArray();
        for (typename Wt::Dbo::collection<Wt::Dbo::ptr<T> >::const_iterator i = c.begin(); i != c.end(); ++i)
        {
            serialize(*i);
        }
        m_writer.EndArray();
        m_pSession = nullptr;
    }

    //非枚举类型
    template<typename T>
    typename std::enable_if< !std::is_enum<T>::value, void>::type
    act(Wt::Dbo::FieldRef<T> field);

    //枚举类型
    template<typename T>
    typename std::enable_if< std::is_enum<T>::value, void>::type
    act(Wt::Dbo::FieldRef<T> field)
    {
        //枚举类型
        m_writer.Key(field.name());
        m_writer.Int(field.value());
    }

    //各种类型
    void act(const Wt::Dbo::FieldRef<std::string>& field)
    {
        m_writer.Key(field.name());
        m_writer.String(field.value());
    }
    void act(const Wt::Dbo::FieldRef<long long>& field)
    {
        m_writer.Key(field.name());
        m_writer.Uint64(field.value());
    }
    void act(const Wt::Dbo::FieldRef<int>& field)
    {
        m_writer.Key(field.name());
        m_writer.Int(field.value());
    }
    void act(const Wt::Dbo::FieldRef<long>& field)
    {
        m_writer.Key(field.name());
        m_writer.Int64(field.value());
    }
    void act(const Wt::Dbo::FieldRef<short>& field)
    {
        m_writer.Key(field.name());
        m_writer.Int(field.value());
    }
    void act(const Wt::Dbo::FieldRef<bool>& field)
    {
        m_writer.Key(field.name());
        m_writer.Bool(field.value());
    }
    void act(const Wt::Dbo::FieldRef<float>& field)
    {
        m_writer.Key(field.name());
        m_writer.Double(field.value());
    }
    void act(const Wt::Dbo::FieldRef<double>& field)
    {
        m_writer.Key(field.name());
        m_writer.Double(field.value());
    }
    void act(const Wt::Dbo::FieldRef<std::chrono::system_clock::time_point>& field)
    {
        m_writer.Key("time_stamp");
        std::time_t t = std::chrono::system_clock::to_time_t(field.value());
        m_writer.Uint64(t);

        std::stringstream ss;
        ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
        m_writer.Key(field.name());
        m_writer.String(ss.str());
    }
    void act(const Wt::Dbo::FieldRef<std::chrono::duration<int, std::milli> >& field)
    {
        //表示时间段
        //m_writer.Key(field.name());
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
        m_writer.Key(field.name());
        if (field.value())
        {
            m_writer.String(field.id());
        }
        else
        {
            m_writer.String("null");
        }
    }

    template<typename T>
    void actWeakPtr(const Wt::Dbo::WeakPtrRef<T>& field)
    {
        m_writer.Key(field.joinName());
        Wt::Dbo::ptr<T> v = field.value().query();
        if (v)
        {
            serialize(v);
        }
        else
        {
            m_writer.String("null");
        }
    }

    template<typename T>
    void actCollection(const Wt::Dbo::CollectionRef<T>& collec)
    {
        if (collec.type() == Wt::Dbo::ManyToOne)
        {
            Wt::Dbo::collection<Wt::Dbo::ptr<T> > c = collec.value();
            m_writer.Key(m_pSession->tableName<T>() + std::string("s"));
            m_writer.StartArray();
            for (typename Wt::Dbo::collection<Wt::Dbo::ptr<T> >::const_iterator i = c.begin(); i != c.end(); ++i)
            {
                m_writer.String(i->id());
                //serialize(*i);
            }
            m_writer.EndArray();
        }
    }

private:
    Wt::Dbo::Session* m_pSession;
    rapidjson::StringBuffer m_buffer;
    rapidjson::Writer<rapidjson::StringBuffer> m_writer;
};

template<typename C>
std::string json_serializer(const C& nStatus, const std::string& strAction, const std::string& strMsg)
{
    JsonSerializer serializer(nStatus, strAction, strMsg);
    serializer.serialize();
    return serializer.GetString();
}

template<typename C>
std::string json_serializer(const C& c, int nStatus, const std::string& strAction, const std::string& strMsg)
{
    JsonSerializer serializer(nStatus, strAction, strMsg);
    serializer.serialize(c);
    return serializer.GetString();
}

template<typename C>
std::string json_serializer(const Wt::Dbo::ptr<C>& c, int nStatus, const std::string& strAction, const std::string& strMsg)
{
    JsonSerializer serializer(nStatus, strAction, strMsg);
    serializer.serialize(c);
    return serializer.GetString();
}

template<typename C>
std::string json_serializer(const std::vector<Wt::Dbo::ptr<C> >& v, int nStatus, const std::string& strAction, const std::string& strMsg)
{
    JsonSerializer serializer(nStatus, strAction, strMsg);
    serializer.serialize(v);
    return serializer.GetString();
}

template<typename C>
std::string json_serializer(const Wt::Dbo::collection<C>& c, int nStatus, const std::string& strAction, const std::string& strMsg)
{
    JsonSerializer serializer(nStatus, strAction, strMsg);
    serializer.serialize(c);
    return serializer.GetString();
}

#endif //_JSONSERIALIZER_H
