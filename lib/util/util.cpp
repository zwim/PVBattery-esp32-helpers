#include <Arduino.h>

namespace util
{
    bool isStringTrue(char str[])
    {
        if (strcmp(str, "off") == 0 ||
            strcmp(str, "false") == 0 ||
            strcmp(str, "0") == 0 ||
            strcmp(str, "disabled") == 0 ||
            strcmp(str, "disable") == 0)
        {
            return false;
        }
        else
            return true;
    }

    void stringToLower(char str[])
    {
        for(int i = 0; str[i]; ++i)
            str[i] = tolower(str[i]);
    }

}

