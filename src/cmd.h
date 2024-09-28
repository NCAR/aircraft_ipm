/********************************************************************
 ** 2024, Copyright University Corporation for Atmospheric Research
 ********************************************************************
*/

#include <string>
#include <map>

#ifndef CMD_H
#define CMD_H

class ipmCmd
{
    public:

        ipmCmd();
        ~ipmCmd();

        void printMenu();
        bool verify(std::string cmd);
        auto response(std::string msg) { return _ipm_commands.find(msg); }

    private:
        // Map message to expected response
        std::map<std::string, std::string>_ipm_commands;

};

#endif /* CMD_H */
