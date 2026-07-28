//
// Created by Lenovo on 2026/1/23.
//

#include "cvi_service_radio.h"

cvi_service_radio::cvi_service_radio(const std::string& process_name, int32_t argc, const char* argv[]) : StandardProcessFramework(process_name, argc, argv)
{
}

cvi_service_radio::~cvi_service_radio()
{
}

cvi::expected<bool, std::string> cvi_service_radio::onPrepare(const std::vector<std::string>& vec_argv)
{
    //预初始化代码填写在这里,不需要可以删除

    // 调用父类方法,如果不需要可以直接return <true>;或return <错误信息>;
    return StandardProcessFramework::onPrepare(vec_argv);
}

cvi::expected<bool, std::string> cvi_service_radio::onInitialize()
{
    //初始化代码填写在这里,不需要可以删除


    // 调用父类方法,如果不需要可以直接return <true>;或return <错误信息>;
    return StandardProcessFramework::onInitialize();
}

cvi::expected<EXPECT_STATE, std::string> cvi_service_radio::onRun()
{
    // 运行代码填写在这里

    // return E_RUN_START_LOOP：则程序自动阻塞，持续运行
    // return E_RUN_FINISH：则程序直接退出
    return E_RUN_START_LOOP;
}

cvi::expected<bool, std::string> cvi_service_radio::onDeinitialize()
{
    // 程序退出时，资源回收代码填写在这里


    // 调用父类方法,如果不需要可以直接return <true>;或return <错误信息>;
    return StandardProcessFramework::onDeinitialize();
}

cvi::expected<bool, std::string> cvi_service_radio::onSignal(int signum)
{
    // 信号处理代码填写在这里,不需要可以删除


    // 调用父类方法,如果不需要可以直接return <true>;或return <错误信息>;
    return StandardProcessFramework::onSignal(signum);
}
