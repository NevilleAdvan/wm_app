//
// Created by Lenovo on 2025/12/26.
//

#include <unistd.h>
#include "cvi_service_radio.h"

std::shared_ptr<StandardProcessFramework> pStandardProcessFramework = nullptr;

std::shared_ptr<StandardProcessFramework>  createStandardProcessFramework(int32_t argc, const char* argv[])
{
    if (pStandardProcessFramework == nullptr)
    {
        std::string process_name = "cvi-service-radio";
        pStandardProcessFramework = std::make_shared<cvi_service_radio>(process_name , argc, argv);
    }
    return pStandardProcessFramework;
}

int32_t main(int32_t argc, const char* argv[])
{
    int32_t iRet = 0;
    std::shared_ptr<StandardProcessFramework> pStandardProcess = createStandardProcessFramework(argc,argv);
    if (pStandardProcess)
    {
        pStandardProcess->launch();
    }else
    {
        printf("pStandardProcess is error,start failed\n");
    }
    return  iRet;
}
