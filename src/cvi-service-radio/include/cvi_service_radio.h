//
// Created by Lenovo on 2026/1/23.
//

#ifndef CVI_SERVICE_RADIO_CVI_SERVICE_RADIO_H
#define CVI_SERVICE_RADIO_CVI_SERVICE_RADIO_H

#include "cvi/cvi-lib-servicefw/StandardProcessFramework.h"

class cvi_service_radio : public StandardProcessFramework
{
public:
    cvi_service_radio(const std::string& process_name , int32_t argc, const char* argv[]);

    ~cvi_service_radio();
    // 预初始化：用于参数解析、日志初始化、配置初始化等
    cvi::expected<bool, std::string> onPrepare(const std::vector<std::string>& vec_argv) override;

    // 进程初始化：仅执行一次
    cvi::expected<bool, std::string> onInitialize() override;

    // 进程运行：核心业务逻辑
    cvi::expected<EXPECT_STATE, std::string> onRun() override;

    // 进程销毁：资源清理
    cvi::expected<bool, std::string> onDeinitialize() override;

    // 信号处理
    cvi::expected<bool, std::string> onSignal(int signum) override;
};


#endif //CVI_SERVICE_RADIO_CVI_SERVICE_RADIO_H