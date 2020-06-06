#include "AuthorizeInstance.h"
#include "model/user.h"
#include "model/userinfo.h"
#include <jwt-cpp/jwt.h>

void AuthorizeInstance::initialization(const cppcms::json::value& settings)
{
    const cppcms::string_key& strIssuer = settings.get<std::string>("authorization.issuer");
    this->m_strIssuer = sha256(jwt::base::encode<jwt::alphabet::base64url>(strIssuer));

    const cppcms::string_key& strSecret = settings.get<std::string>("authorization.secret");
    this->m_strSecret = md5(jwt::base::encode<jwt::alphabet::base64url>(strSecret));
}

cppcms::string_key AuthorizeInstance::create_token(const dbo::ptr<User>& pUser,
                                                   const cppcms::string_key& strSubject,
                                                   unsigned unMinutes)
{
    if (!pUser)
    {
        return cppcms::string_key("");
    }

    jwt::builder token = jwt::create()
            .set_issuer(m_strIssuer)
            .set_expires_at(std::chrono::system_clock::now() + std::chrono::minutes{unMinutes})
            .set_subject(strSubject)
            .set_audience(pUser.id())
            .set_not_before(std::chrono::system_clock::now())
            .set_issued_at(std::chrono::system_clock::now())
            .set_id(pUser.id())
            .set_type("JWT");

    switch (pUser->getUserRole())
    {
        case UserRole::MEMBER :
        case UserRole::GENERAL :
        case UserRole::ADMINISTRATOR :
            token.set_payload_claim("role", jwt::claim(picojson::value((int64_t)pUser->getUserRole())));
            break;
        case UserRole::GUEST:
            break;
    }

    return token.sign(jwt::algorithm::hs256{m_strSecret});
}

std::pair<bool, cppcms::string_key> AuthorizeInstance::verify_token(const cppcms::string_key& strToken)
{
    try
    {
        jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{m_strSecret})
                .with_issuer(m_strIssuer)
                .verify(jwt::decoded_jwt(strToken));
    }
    catch (const std::exception& ex)
    {
        return std::make_pair<bool, std::string>(false, std::string(ex.what()));
    }
    return std::make_pair<bool, std::string>(true, std::string());
}

cppcms::string_key AuthorizeInstance::get_user_id(const cppcms::string_key& strToken)
{
    try
    {
        jwt::decoded_jwt decoded(strToken);
        return decoded.get_payload_claim("jti").as_string();
    }
    catch (const std::exception& ex)
    {
        return cppcms::string_key();
    }
}

bool AuthorizeInstance::is_admin( const cppcms::string_key& strToken )
{
    try
    {
        jwt::decoded_jwt decoded(strToken);
        return decoded.get_payload_claim("role").as_int() == (int64_t)UserRole::ADMINISTRATOR;
    }
    catch (const std::exception& ex)
    {
        return false;
    }
}

uint64_t AuthorizeInstance::get_role(const std::string &strToken) const
{
    try
    {
        jwt::decoded_jwt decoded(strToken);
        return decoded.get_payload_claim("role").as_int();
    }
    catch (const std::exception& ex)
    {
        return 0;
    }
}

