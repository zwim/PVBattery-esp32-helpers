#ifndef WEBSERVER_h
#define WEBSERVER_h

namespace webServer
{
    extern IPAddress local_ip;
    extern IPAddress gateway_ip;
    extern char gateway[20];
    void init();
    void readConfigFile();
    void saveConfigFile();
    void initWebserverFunctions();
    bool checkGateway();
}

#endif
