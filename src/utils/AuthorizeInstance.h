#ifndef AUTHORIZEUTIL_H
#define AUTHORIZEUTIL_H

#include "define.h"
#include "singleton.h"
#include <cppcms/json.h>
#include <cppcms/string_key.h>
#include <utility>

class AuthorizeInstance : public Singleton<AuthorizeInstance>
{
public:
    friend Singleton<AuthorizeInstance>;

    void initialization(const cppcms::json::value& settings);
    cppcms::string_key create_token(const dbo::ptr<User>& pUser,
                                    const cppcms::string_key& strSubject = cppcms::string_key("Authorize"),
                                    unsigned unMinutes = 12 * 60);
    std::pair<bool, cppcms::string_key> verify_token(const cppcms::string_key& strToken);
    cppcms::string_key get_user_id( const cppcms::string_key& strToken );
    bool is_admin( const cppcms::string_key& strToken );
    uint64_t get_role(const std::string& strToken) const;

private:
    AuthorizeInstance() = default;
    ~AuthorizeInstance() override = default;
    AuthorizeInstance(const AuthorizeInstance&) = delete;
    AuthorizeInstance(const AuthorizeInstance&&) = delete;
    AuthorizeInstance& operator=(const AuthorizeInstance&) = delete;
    AuthorizeInstance& operator=(const AuthorizeInstance&&) = delete;

private:
    cppcms::string_key m_strIssuer;
    cppcms::string_key m_strSecret;
};

#endif // AUTHORIZEUTIL_H
