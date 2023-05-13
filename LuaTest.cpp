
#include <iostream>
#include <string>

#if !defined(__GNUC__)
#define NOMINMAX
#endif
#if !defined (_WIN32) && !defined (_WIN64)
#include <locale>
#include <iomanip>
#include <codecvt>
#include <fcntl.h>
#include <unistd.h>
void OutputDebugStringA(const char* pOut)
{   // mkfifo /tmp/dbgout  ->  tail -f /tmp/dbgout
    int fdPipe = open("/tmp/dbgout", O_WRONLY | O_NONBLOCK);
    if (fdPipe >= 0)
    {
        std::string strTmp(pOut);
        write(fdPipe, strTmp.c_str(), strTmp.size());
        close(fdPipe);
    }
}
const char*  const szScriptPath{"../script4.lua"};
#else
#include <Windows.h>
const char*  const szScriptPath{"../../script4.lua"};
#endif
#include "LuaScript.h"


int main(int argc, const char* argv[])
{
#if !defined(__GNUC__) && (defined(_WIN32) || defined(_WIN64))
    // Detect Memory Leaks
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
    //_setmode(_fileno(stdout), _O_U16TEXT);
#endif

    // Recursive lambda function to extract the lua parameter struct into a vector of strings
    std::function<void(ScrPara_t&, std::vector<std::string>&)> fnPopParameter = [&fnPopParameter](ScrPara_t& Param, std::vector<std::string>& vResStrings)
    {
        if (std::holds_alternative<bool>(Param))
            vResStrings.emplace_back(std::to_string(std::get<bool>(Param)));
        else if (std::holds_alternative<int>(Param))
            vResStrings.emplace_back(std::to_string(std::get<int>(Param)));
        else if (std::holds_alternative<double>(Param))
            vResStrings.emplace_back(std::to_string(std::get<double>(Param)));
        else if (std::holds_alternative<std::string>(Param))
            vResStrings.emplace_back(std::get<std::string>(Param));
        else if (std::holds_alternative<void*>(Param))
            vResStrings.emplace_back(std::to_string(reinterpret_cast<size_t>(std::get<void*>(Param))));
        else if (std::holds_alternative<std::vector<ScrPara_t>>(Param))
        {
            std::vector<ScrPara_t> vecParam = std::get<std::vector<ScrPara_t>>(Param);
            for(auto& var : vecParam)
                fnPopParameter(var, vResStrings);
        }
        else
            static_assert(true, "parameter typ not suported!");
    };

    LuaScript Script;
    Script.RegisterFunction("DbgOut", [](ScrPara_t InPara) -> ScrPara_t
    {
        if (std::holds_alternative<std::string>(InPara) == true)
            OutputDebugStringA(std::string("Event function call returned: " + std::get<std::string>(InPara) + "\r\n").c_str());

        return true;
    });
    Script.SetGlobalValue("_VERSION_", ScrPara_t{1.0});

    if (Script.LoadScriptFile(szScriptPath) == true)
    {
        // call a init function in the script
        ScrPara_t OutDummy;
        Script.CallFunction("Init", ScrPara_t(), 0, OutDummy);

        // set a global variable in the script
        Script.SetGlobalValue("CountWordsCount", ScrPara_t{10});

        ScrPara_t InParam = std::vector<ScrPara_t>({ std::string("CMakeCache.txt"), 0 }); // the 0 is just a int, what is the thread number in the call below
        ScrPara_t OutPara;
        if (Script.CallFunction("CountWords", InParam, 1, OutPara) == true)
        {
            std::vector<std::string> vResStrings;
            fnPopParameter(OutPara, vResStrings);
            for (size_t n = 0; n + 1 < vResStrings.size(); n += 2)
                OutputDebugStringA(std::string(vResStrings[n] + " = " + vResStrings[n + 1] + "\r\n").c_str());
        }

        // Call a lua function with a map of strings. The lua function will output the parameter in OutputDebugString
        ParaTable_t tblTest;
        std::vector<std::string> vTmp1({"Welt", "wie","geht","es"});
        tblTest.emplace("Hallo", vTmp1);
        std::vector<std::string> vTmp2({"World","how","are","you","today"});
        tblTest.emplace("Hi", vTmp2);

        InParam = tblTest;
        if (Script.CallFunction("OutTable", InParam, 0, OutPara) == false)
            OutputDebugStringA(std::string(Script.GetErrorString() + "\r\n").c_str());
    }

    ScrPara_t OutPara;
    if (Script.NotifyEvent("EvDoSomething", ScrPara_t({std::vector<ScrPara_t>({ 3, 5})}), 1, OutPara) == true)
    {
        if (std::holds_alternative<int>(OutPara) == true)
            OutputDebugStringA(std::string("Event function call returned: " + std::to_string(std::get<int>(OutPara)) + "\r\n").c_str());
    }

    // start 20 threads all calling a lua function
    constexpr size_t nAnzThreads = 20;
    std::thread t[nAnzThreads];
    std::mutex mxLock;

    for (size_t n = 0; n < nAnzThreads; ++n)
    {
        t[n] = std::thread([&] (size_t iThreadNr)
        {
            ScrPara_t InParam = std::vector<ScrPara_t>({ std::string("CMakeCache.txt"), static_cast<int>(iThreadNr) });
            ScrPara_t OutPara;
            if (Script.CallFunction("CountWords", InParam, 1, OutPara) == true)
            {
                std::lock_guard<std::mutex> lock(mxLock);
                OutputDebugStringA(std::string("Has the function call return a List?: " + std::to_string(std::holds_alternative<std::vector<ScrPara_t>>(OutPara)) + "\r\n").c_str());
            }
            else
            {
                std::lock_guard<std::mutex> lock(mxLock);
                OutputDebugStringA(std::string("Lua  call error: " + Script.GetErrorString() + "\r\n").c_str());
            }
        }, n);
    }

    for (size_t n = 0; n < nAnzThreads; ++n)
    {
        if (t[n].joinable())
            t[n].join();
    }

    if (Script.NotifyEvent("EvDoSomething", ScrPara_t({std::vector<ScrPara_t>({ 7, 8})}), 1, OutPara) == true)
    {
        if (std::holds_alternative<int>(OutPara) == true)
            OutputDebugStringA(std::string("Event function call returned: " + std::to_string(std::get<int>(OutPara)) + "\r\n").c_str());
    }

    OutPara = Script.GetGlobalValue("CountWordsCount");
    if (std::holds_alternative<int>(OutPara) == true)
        OutputDebugStringA(std::string("Global variable holds: " + std::to_string(std::get<int>(OutPara)) + "\r\n").c_str());

    Script.CallFunction("Fini", ScrPara_t(), 0, OutPara);

    return 0;
}
