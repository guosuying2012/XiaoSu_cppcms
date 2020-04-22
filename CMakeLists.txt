cmake_minimum_required(VERSION 3.10.2)

project(xiaosu)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

include_directories(src/)

add_executable(xiaosu
        src/main.cpp
        src/define.h
        src/model/systemlog.h
        src/model/user.h
        src/model/userinfo.h
        src/model/article.h
        src/model/category.h
        src/service/BaseService.h
        src/service/BaseService.cpp
        src/service/ApplicationService.h
        src/service/ApplicationService.cpp
        src/service/ArticleService.h
        src/service/ArticleService.cpp
        src/service/UserService.h
        src/service/UserService.cpp
        src/service/CategoryService.h
        src/service/CategoryService.cpp
        src/utils/md5.h
        src/utils/md5.cpp
        src/utils/sha256.h
        src/utils/sha256.cpp
        src/utils/singleton.h
        src/utils/DboAppender.h
        src/utils/DboInstence.h
        src/utils/JsonSerializer.h
        src/utils/JsonDeserializer.h
        src/utils/AuthorizeInstance.h
        src/utils/AuthorizeInstance.cpp)

target_link_libraries(xiaosu booster cppcms curl cpr wtdbo wtdbomysql
        /usr/local/lib/libMailCore.a etpan xml2 sasl2 tidy ctemplate ssl crypto icudata
        icui18n icuio icu-le-hb iculx icutest icutu icuuc pthread uuid glib-2.0 uchardet z)
