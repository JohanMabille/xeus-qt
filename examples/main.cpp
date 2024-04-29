#include <iostream>
#include <fstream>

#include <QApplication>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

#include <thread>

#include "xq/xq_server.hpp"
#include "xq/xq_qt_poller.hpp"

#include "xeus/xkernel.hpp"
#include "xeus/xkernel_configuration.hpp"

#include "xeus-lua/xinterpreter.hpp"

void create_json_file(std::string kernel_info)
{
    
    std::ofstream kernel_json("kernel.json");
    kernel_json << kernel_info;
    kernel_json.close();
}


class MainWindow : public QMainWindow{
public:

    using interpreter_ptr = std::unique_ptr<xlua::interpreter>;
    using history_manager_ptr = std::unique_ptr<xeus::xhistory_manager>;

    MainWindow()
    : QMainWindow()
    {
        // zmq context
        auto context = xeus::make_context<zmq::context_t>();

        // Create interpreter instance
        interpreter_ptr interpreter = std::make_unique<xlua::interpreter>();


        // Create history instance
        history_manager_ptr hist = xeus::make_in_memory_history_manager();


        p_kernel.reset(new xeus::xkernel(xeus::get_user_name(),
                         std::move(context),
                         std::move(interpreter),
                         make_xq_server,
                         std::move(hist),
                         nullptr));

        // start the kernel
        p_kernel->start();

    }

    void closeEvent(QCloseEvent*) override
    {
        std::cout<<"closing the mainWindow\n";
        p_kernel->get_server().stop();
    }

    xeus::xkernel & get_kernel(){
        return *p_kernel;
    }

private:
    std::unique_ptr<xeus::xkernel> p_kernel;
};

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    MainWindow mainWindow;
    const auto& config = mainWindow.get_kernel().get_config();

    std::string kernel_info, tutorial;
    tutorial = "Starting xeus-python kernel...\n\n"
                    "If you want to connect to this kernel from an other client, just copy"
                    " and paste the following content inside of a `kernel.json` file. And then run for example:\n\n"
                    "# jupyter console --existing kernel.json\n\n"
                    "kernel.json\n\n";
    kernel_info = "{\n\n"
                  "    \"transport\": \"" + config.m_transport + "\",\n"
                  "    \"ip\": \"" + config.m_ip + "\",\n"
                  "    \"control_port\": " + config.m_control_port + ",\n"
                  "    \"shell_port\": " + config.m_shell_port + ",\n"
                  "    \"stdin_port\": " + config.m_stdin_port + ",\n"
                  "    \"iopub_port\": " + config.m_iopub_port + ",\n"
                  "    \"hb_port\": " + config.m_hb_port + ",\n"
                  "    \"signature_scheme\": \"" + config.m_signature_scheme + "\",\n"
                  "    \"key\": \"" + config.m_key + "\"\n"
                  "}\n";

    mainWindow.resize(620, 440);

    QLabel* label = new QLabel(&mainWindow);
    label->setText(QString::fromStdString(tutorial + kernel_info));
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setWordWrap(true);

    QPushButton btn("kernel", &mainWindow);
    QObject::connect(&btn, &QPushButton::clicked, [&](){create_json_file(kernel_info);});

    mainWindow.setCentralWidget(label);

    mainWindow.show();
    application.exec();

    return 0;
}
