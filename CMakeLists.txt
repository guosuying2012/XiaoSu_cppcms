cmake_minimum_required(VERSION 3.10.2)

project(xiaosu)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

add_executable(xiaosu src/main.cpp src/define.h
        src/model/systemlog.h src/model/user.h src/model/userinfo.h
        src/model/article.h src/model/category.h
        src/service/BaseService.cpp src/service/BaseService.h
        src/service/ApplicationService.h src/service/ApplicationService.cpp
        src/service/ArticleService.cpp src/service/ArticleService.h
        src/utils/DboAppender.h src/utils/DboInstence.h
        src/utils/JsonSerializer.h src/utils/JsonUnserializer.h)

target_link_libraries(xiaosu booster cppcms curl cpr wtdbo wtdbomysql
        /usr/local/lib/libMailCore.a etpan xml2 sasl2 tidy ctemplate ssl crypto icudata
        icui18n icuio icu-le-hb iculx icutest icutu icuuc pthread uuid glib-2.0 uchardet z)
