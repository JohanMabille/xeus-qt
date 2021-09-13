#include <iostream>

#include <QApplication>
#include <QLabel>
#include <QMainWindow>
#include <QString>

#include "xq/xqServer.hpp"

#include "xeus/xkernel.hpp"
#include "xeus/xkernel_configuration.hpp"

#include "xeus-sqlite/xeus_sqlite_interpreter.hpp"
#include "xeus-sqlite/xeus_sqlite_config.hpp"

int main(int argc, char *argv[])
{
    // Create interpreter instance
    using interpreter_ptr = std::unique_ptr<xeus_sqlite::interpreter>;
    interpreter_ptr interpreter = std::make_unique<xeus_sqlite::interpreter>();

    // Create history instance
    using history_manager_ptr = std::unique_ptr<xeus::xhistory_manager>;
    history_manager_ptr hist = xeus::make_in_memory_history_manager();

    // Create xkernel using a Qt compatible server
    xeus::xkernel kernel("mari-sqlite",
                         std::move(interpreter),
                         std::move(hist),
                         nullptr,
                         make_xqServer);

    const auto& config = kernel.get_config();
    std::string kernel_info;
    kernel_info = "Starting xeus-python kernel...\n\n"
                    "If you want to connect to this kernel from an other client, just copy"
                    " and paste the following content inside of a `kernel.json` file. And then run for example:\n\n"
                    "# jupyter console --existing kernel.json\n\n"
                    "kernel.json\n```\n{\n"
                    "    \"transport\": \"" + config.m_transport + "\",\n"
                    "    \"ip\": \"" + config.m_ip + "\",\n"
                    "    \"control_port\": " + config.m_control_port + ",\n"
                    "    \"shell_port\": " + config.m_shell_port + ",\n"
                    "    \"stdin_port\": " + config.m_stdin_port + ",\n"
                    "    \"iopub_port\": " + config.m_iopub_port + ",\n"
                    "    \"hb_port\": " + config.m_hb_port + ",\n"
                    "    \"signature_scheme\": \"" + config.m_signature_scheme + "\",\n"
                    "    \"key\": \"" + config.m_key + "\"\n"
                    "}\n```";

    QApplication application(argc, argv);
    kernel.start();
    std::cout << kernel_info;
    QMainWindow mainWindow;
    mainWindow.resize(320, 240);
    QLabel *label = new QLabel(&mainWindow);
    label->setText(QString::fromStdString(kernel_info));
    mainWindow.show();
    return application.exec();
    // label.setWordWrap(true);
}