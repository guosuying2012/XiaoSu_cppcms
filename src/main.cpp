#include "utils/DboAppender.h"
#include "service/ApplicationService.h"

#include <cppcms/service.h>
#include <cppcms/applications_pool.h>

int main(int argc, char* argv[])
{
    try
    {
        static DboAppender dbo;
        cppcms::service srv(argc,argv);
        DboSingleton::GetInstance(srv.settings());
        srv.applications_pool().mount(cppcms::applications_factory<ApplicationService>());
        plog::init<plog::TxtFormatter>(plog::verbose, "runtime.log").addAppender(&dbo);
        srv.run();
    }
    catch (const std::exception& ex)
    {
        PLOG_ERROR << "SystemError: " << ex.what();
        return -1;
    }
}
