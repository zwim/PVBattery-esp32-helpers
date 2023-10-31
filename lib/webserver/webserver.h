//#ifndef WEBSERVER_H
//#define WEBSERVER_H

namespace webServer
{
    void init();
    void readConfigFile();
    void saveConfigFile();
    void initWebserverFunctions();
    void looper();
}

//#endif