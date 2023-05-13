
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
#else
#include <Windows.h>
#endif
#include "LuaScript.h"

/*
bool CallLuaScript(std::string& strMailFile, std::map<std::string, std::vector<std::string>>& tblMailUser, bool bInOut)
{
    bool result = true;

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    lua_pushcfunction(L, [](lua_State *L) -> int
    {
        const char* caStr = luaL_checkstring(L, 1);
        if (caStr != nullptr)
            OutputDebugStringA(caStr);
        return 1;
    });
    //lua_pushcfunction(L, fnOutDebugString);

    // Get the value on top of the stack
    // and set as a global, in this case is the function
    lua_setglobal(L, "OutputDebugString");

    if (luaL_loadfile(L, "D:\\Users\\Thomas\\Documents\\Visual Studio 2013\\Projects\\Jana3\\build\\Debug\\script4.lua") == LUA_OK) {
        if (lua_pcall(L, 0, 1, 0) == LUA_OK) {  // Executes the script
            lua_pop(L, lua_gettop(L));          // removes the return element from the Stack
        }
    }

    int nStackSize = lua_gettop(L);

    // Put the function to be called onto the stack
    if (bInOut == true) // Mail received from smtpd
        lua_getglobal(L, "HaveMailToSend");
    else
        lua_getglobal(L, "HaveMailReceived");
    if (lua_isfunction(L, -1)) {
        lua_pushlstring(L, &strMailFile[0], strMailFile.size());  // first argument
        //lua_pushinteger(L, 4);  // second argument
*/
/*
    lua_newtable(L);              // table
    lua_pushstring(L,"User1");
    lua_rawseti(L,-2,1);
    lua_pushstring(L,"User2");
    lua_rawseti(L,-2,2);

    lua_newtable(L);              // table
    lua_pushstring(L, "User1");    // table,key
    lua_pushstring(L, "thomas@fam-hauck.de"); // table,key,value
    lua_settable(L,-3);         // table
    lua_pushstring(L, "User2");    // table,key
    lua_pushstring(L, "thauck@janaserver.de"); // table,key,value
    lua_settable(L,-3);         // table
*/
/*
    lua_newtable(L);        // table
    int iRow = 0;
    for (auto& itMailUser : tblMailUser)
    {
        lua_newtable(L);    // table row
        lua_pushstring(L, &itMailUser.first[0]);    // Col 1
        lua_rawseti(L,-2,1);
        lua_pushstring(L, &itMailUser.second[0][0]);// Col 2
        lua_rawseti(L,-2,2);

        lua_rawseti(L,-2, ++iRow);
    }
*//*
    lua_newtable(L);              // table
    lua_pushstring(L,"User1");
    lua_rawseti(L,-2,1);
    lua_pushstring(L,"thomas@fam-hauck.de");
    lua_rawseti(L,-2,2);

    lua_rawseti(L,-2,1);

    lua_newtable(L);              // table
    lua_pushstring(L,"User2");
    lua_rawseti(L,-2,1);
    lua_pushstring(L,"thauck@janaserver.de");
    lua_rawseti(L,-2,2);

    lua_rawseti(L,-2,2);
*/
/*
        // Execute my_function with 2 arguments and 1 return value
        if (lua_pcall(L, 2, 1, 0) == LUA_OK) {

            // Check if the return is an integer
            //if (lua_isinteger(L, -1)) {
            if (lua_isboolean(L, -1)) {

                // Convert the return value to integer
                //int64_t result = lua_tointeger(L, -1);
                result = lua_toboolean(L, -1);

                // Pop the return value
                lua_pop(L, 1);
                OutputDebugStringA(string("Result: " + to_string(result) + "\r\n").c_str());
            }
        }
        else
        {
             std::string strLuaErr = lua_tostring(L, -1);
            OutputDebugStringA(string("Lua error: " + strLuaErr + "\r\n").c_str());
            // Pop the error value
            lua_pop(L, 1);
        }
        // Remove the function from the stack
        lua_pop(L, lua_gettop(L));
    }

    if (nStackSize != lua_gettop(L))
        OutputDebugStringA(string("Lua stack error, stacksize old: " + to_string(nStackSize) + ", stacksize neu: " + to_string(lua_gettop(L)) + "\r\n").c_str());

    lua_close(L);

    return result;
}
*/
int main(int argc, const char* argv[])
{
#if !defined(__GNUC__) && (defined(_WIN32) || defined(_WIN64))
    // Detect Memory Leaks
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
    //_setmode(_fileno(stdout), _O_U16TEXT);
#endif

    LuaScript Script;
    Script.RegisterFunction("DbgOut", [](ScrPara_t InPara) -> ScrPara_t
    {
        if (std::holds_alternative<std::string>(InPara) == true)
            OutputDebugStringA(std::string("Eventfunction call returned: " + std::get<std::string>(InPara) + "\r\n").c_str());

        return true;
    });
    Script.SetGlobalValue("_VERSION_", ScrPara_t{1.0});

    if (Script.LoadScriptFile("../../script4.lua") == true)
    {
        ScrPara_t OutDummy;
        Script.CallFunction("Init", ScrPara_t(), 0, OutDummy);

        Script.SetGlobalValue("CountWordsCount", ScrPara_t{10});

        ScrPara_t InParam = std::vector<ScrPara_t>({ std::string("../../lualock.h"), 2 });
        ScrPara_t OutPara;
        if (Script.CallFunction("CountWords", InParam, 1, OutPara) == true)
        {
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

                /*std::visit([&fnPopParameter, &vResStrings](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, int> || std::is_same_v<T, double>)
                        vResStrings.emplace_back(std::to_string(arg));
                    else if constexpr (std::is_same_v<T, std::string>)
                        vResStrings.emplace_back(arg);
                    else if constexpr (std::is_same_v<T, void*>)
                        vResStrings.emplace_back(std::to_string(reinterpret_cast<size_t>(arg)));
                    else if constexpr (std::is_same_v<T, ParaTable_t>)
                        ;
                    else if constexpr (std::is_same_v<T, std::vector<ScrPara_t>>)
                    {
                        for(auto& var : arg)
                            fnPopParameter(var, vResStrings);
                    }
                    else
                        static_assert(always_false_v<T>, "parameter typ not suported!");
                }, Param);*/
            };
            std::vector<std::string> vResStrings;
            fnPopParameter(OutPara, vResStrings);
            for (size_t n = 0; n + 1 < vResStrings.size(); n += 2)
                OutputDebugStringA(std::string(vResStrings[n] + " = " + vResStrings[n + 1] + "\r\n").c_str());
        }

        ParaTable_t tblTest;
        std::vector<std::string> vTmp1({"Welt", "wie","geht","es"});
        tblTest.emplace("Hallo", vTmp1);
        std::vector<std::string> vTmp2({"World","how","are","you","today"});
        tblTest.emplace("Hi", vTmp2);

        InParam = tblTest;
        if (Script.CallFunction("OutTable", InParam, 0, OutPara) == false)
            OutputDebugStringA(std::string(Script.GetErrorString() + "\r\n").c_str());
    }
//
//    lua_State *L = luaL_newstate();
//    luaL_openlibs(L);
//
//    lua_pushcfunction(L, [](lua_State *L) -> int
//    {
//        const char* caStr = luaL_checkstring(L, 1);
//        if (caStr != nullptr)
//            OutputDebugStringA(caStr);
//        return 1;
//    });
//    lua_setglobal(L, "OutputDebugString");
//
//    if (luaL_loadfile(L, "D:\\Users\\Thomas\\Documents\\Visual Studio 2013\\Projects\\Jana3\\build\\Debug\\script4.lua") == LUA_OK)
//    {
//        if (lua_pcall(L, 0, 1, 0) == LUA_OK)    // Executes the script
//            lua_pop(L, lua_gettop(L));          // removes the return element from the Stack
//    }
//
//    int nStackSize = lua_gettop(L);


    ScrPara_t OutPara;
    if (Script.NotifyEvent("EvDoSomething", ScrPara_t({std::vector<ScrPara_t>({ 3, 5})}), 1, OutPara) == true)
    {
        if (std::holds_alternative<int>(OutPara) == true)
            OutputDebugStringA(std::string("Eventfunction call returnd: " + std::to_string(std::get<int>(OutPara)) + "\r\n").c_str());
    }

    constexpr size_t nAnzThreads = 20;
    std::thread t[nAnzThreads];
    std::mutex mxLock;

    for (size_t n = 0; n < nAnzThreads; ++n)
    {
        t[n] = std::thread([&] (size_t iThreadNr)
        {
            ScrPara_t InParam = std::vector<ScrPara_t>({ std::string("../../LuaTest.cpp"), static_cast<int>(iThreadNr) });
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


/*            mxLock.lock();
            lua_State *LThread = lua_newthread(L);
            int nSsNach = lua_gettop(L);
            OutputDebugStringA(std::string("ThreadNr.: " + std::to_string(iThreadNr) + ", gettop-Index: " + std::to_string(nSsNach) + ", lua_State*: " + std::to_string(reinterpret_cast<uint64_t>(LThread)) + "\r\n").c_str());
//            lua_State *LThread2 = lua_tothread(L, nSsNach);
//            if (LThread != LThread2)
//                OutputDebugStringA("Checkpoint\r\n");

            int nStackSizeThread = lua_gettop(LThread);
            mxLock.unlock();

            lua_getglobal(LThread, "CountWords");
            if (lua_isfunction(LThread, -1))
            {
                //lua_pushlstring(LThread, "Hallo Welt, wie geht es Hallo dir?", 34);  // first argument
                std::string strFileName("D:\\Users\\Thomas\\Documents\\Visual Studio 2013\\Projects\\Jana3\\src\\ScannEmail.cpp");
                lua_pushlstring(LThread, &strFileName[0], strFileName.size());
                lua_pushinteger(LThread, iThreadNr);

                // Execute my_function with 2 arguments and 1 return value
                if (lua_pcall(LThread, 2, 1, 0) == LUA_OK) {

                    int nStackHeight = lua_gettop(LThread);
                    std::string strRetType = lua_typename(LThread, lua_type(LThread, -1));
                    OutputDebugStringA(std::string("Anzahl Results: " + std::to_string(nStackHeight) +  ", Result type: " + strRetType + "\r\n").c_str());

                    if (lua_istable(LThread, -1)) {
//
//                        lua_pushnil(LThread);
//                        while(lua_next(LThread, -2) != 0)
//                        {
//                            if(lua_isstring(LThread, -1))
//                                OutputDebugStringA(std::string(std::to_string(iThreadNr) + ": " + lua_tostring(LThread, -2) + std::string(" = ") + lua_tostring(LThread, -1) + std::string("\r\n")).c_str());
//                            else if(lua_isnumber(LThread, -1))
//                                OutputDebugStringA(std::string(std::to_string(iThreadNr) + ": " + lua_tostring(LThread, -2) + std::string(" ") + std::to_string(lua_tonumber(LThread, -1)) + std::string("\r\n")).c_str());
//                            lua_pop(LThread, 1);
//                        }
//
                        // Pop the return value
                        lua_pop(LThread, 1);
        //                OutputDebugStringA(std::string("Result: " + std::to_string(result) + "\r\n").c_str());
                    }
                }
                else
                {
                    std::string strLuaErr = lua_tostring(LThread, -1);
                    OutputDebugStringA(std::string("Lua error: " + strLuaErr + "\r\n").c_str());
                    // Pop the error value
                    lua_pop(LThread, 1);
                }
                // Remove the function from the stack
                int iTemp = lua_gettop(LThread);
                lua_pop(LThread, lua_gettop(LThread));
            }

            if (nStackSizeThread != lua_gettop(LThread))
                OutputDebugStringA(std::string("Lua stack error in Thread, stacksize old: " + std::to_string(nStackSizeThread) + ", stacksize neu: " + std::to_string(lua_gettop(LThread)) + "\r\n").c_str());

            // Pop the thread from the Stack
            mxLock.lock();
            int iStSize = lua_gettop(L);
            for (int i = 1; i <= iStSize; ++i)
            {
                if (lua_tothread(L, i) == LThread)
                {
                    lua_remove(L, i);
                    OutputDebugStringA(std::string("ThreadNr.: " + std::to_string(iThreadNr) + ", gettop-Index: " + std::to_string(iStSize) + ", lua_State*: " + std::to_string(reinterpret_cast<uint64_t>(LThread)) + "\r\n").c_str());
                    break;
                }
            }
//            while (lua_tothread(L, -1) != LThread)
//            {
//                mxLock.unlock();
//                std::this_thread::sleep_for(std::chrono::milliseconds(10));
//                mxLock.lock();
//            }
//            int iStSize = lua_gettop(L);
//            OutputDebugStringA(std::string("ThreadNr.: " + std::to_string(iThreadNr) + ", gettop-Index: " + std::to_string(iStSize) + ", lua_State*: " + std::to_string(reinterpret_cast<uint64_t>(LThread)) + "\r\n").c_str());
//            lua_pop(L, 1);
            mxLock.unlock();*/
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
            OutputDebugStringA(std::string("Eventfunction call returnd: " + std::to_string(std::get<int>(OutPara)) + "\r\n").c_str());
    }

    OutPara = Script.GetGlobalValue("CountWordsCount");
    if (std::holds_alternative<int>(OutPara) == true)
        OutputDebugStringA(std::string("Global variable holds: " + std::to_string(std::get<int>(OutPara)) + "\r\n").c_str());

    Script.CallFunction("Fini", ScrPara_t(), 0, OutPara);

//
//    if (nStackSize != lua_gettop(L))
//        OutputDebugStringA(std::string("Lua stack error, stacksize old: " + std::to_string(nStackSize) + ", stacksize neu: " + std::to_string(lua_gettop(L)) + "\r\n").c_str());
//
//    lua_close(L);
//    LuaUnlock(nullptr);
//
    return 0;
}
