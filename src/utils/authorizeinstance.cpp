#include "authorizeinstance.h"
#include "define.h"
#include<jwt-cpp/jwt.h>

void AuthorizeInstance::initialization(const cppcms::json::value& settings)
{
    const cppcms::string_key& strIssuer = settings.get<std::string>("authorization.issuer");
    this->m_strIssuer = sha256(jwt::base::encode<jwt::alphabet::base64url>(strIssuer));

    const cppcms::string_key& strSecret = settings.get<std::string>("authorization.secret");
    this->m_strSecret = md5(jwt::base::encode<jwt::alphabet::base64url>(strSecret));
}

cppcms::string_key AuthorizeInstance::create_token(const cppcms::string_key& strUserId,
                                               const cppcms::string_key& strSubject)
{
    return  jwt::create()
            .set_issuer(m_strIssuer)
            .set_expires_at(std::chrono::system_clock::now() + std::chrono::minutes{60})
            .set_subject(strSubject)
            .set_audience(strUserId)
            .set_not_before(std::chrono::system_clock::now())
            .set_issued_at(std::chrono::system_clock::now())
            .set_id(strUserId)
            .set_type("JWT")
            .sign(jwt::algorithm::hs256{m_strSecret});
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
        return decoded.get_payload_claim("admin").as_bool();
    }
    catch (const std::exception& ex)
    {
        return false;
    }
}
