
#include <string>
#include "LuaScript.h"

#if !defined(__GNUC__)
#define NOMINMAX
#endif
#if defined (_WIN32) || defined (_WIN64)
#include <Windows.h>
#else
extern void OutputDebugStringA(const char*);
#endif

std::mutex LuaScript::s_mxLock;
std::mutex LuaScript::s_mxThrLck;
thread_local std::string LuaScript::s_strLuaErr;

void LuaLock(lua_State * L)
{
    LuaScript::LuaLock(L);
}

void LuaUnlock(lua_State * L)
{
    LuaScript::LuaUnlock(L);
}

LuaScript::LuaScript() : m_bScriptLoaded(false), m_NextEvtNr(10000), m_NextRegFnNr(10000)
{
    m_ThreadId = std::this_thread::get_id();
    m_Lua = luaL_newstate();
    luaL_openlibs(m_Lua);

    lua_pushlightuserdata(m_Lua, this);
    lua_setglobal(m_Lua, "__this__");

    lua_pushcfunction(m_Lua, [](lua_State *Lua) -> int
    {
        const char* caStr = luaL_checkstring(Lua, 1);
        if (caStr != nullptr)
            OutputDebugStringA(caStr);
        return 0;
    });

    // Get the value on top of the stack
    // and set as a global, in this case is the function
    lua_setglobal(m_Lua, "OutputDebugString");

    lua_pushcfunction(m_Lua, [](lua_State *Lua) -> int
    {
        int nCountParam = lua_gettop(Lua);
        if (nCountParam != 2 || lua_isstring(Lua, 1) == false || lua_isfunction(Lua, 2) == false)
        {
            OutputDebugStringA("incorrect argument\r\n");
            return luaL_error(Lua, "incorrect argument"); // Jumps back -> is like a throw
        }
        const std::string strEvent(luaL_checkstring(Lua, 1));
        //lua_CFunction fnFunc = lua_tocfunction(Lua, 2);
        int callback_reference = luaL_ref(Lua, LUA_REGISTRYINDEX);

        lua_getglobal(Lua, "__this__");
        if (!lua_islightuserdata(Lua, -1))
        {
            OutputDebugStringA("internel error\r\n");
            return luaL_error(Lua, "internel error"); // Jumps back -> is like a throw
        }
        LuaScript* pThis = reinterpret_cast<LuaScript*>(lua_touserdata(Lua, -1));
        std::lock_guard<std::mutex> lock(s_mxThrLck);
        pThis->m_ummCbFunc.emplace(++pThis->m_NextEvtNr, make_pair(strEvent, callback_reference));
        lua_pop(Lua, 1);

        lua_pushinteger(Lua, pThis->m_NextEvtNr);  /* push result */
        return 1;
    });

    // Get the value on top of the stack
    // and set as a global, in this case is the function
    lua_setglobal(m_Lua, "RegisterEventNotify");

    lua_pushcfunction(m_Lua, [](lua_State *Lua) -> int
    {
        int nCountParam = lua_gettop(Lua);
        if (nCountParam != 1 || lua_isinteger(Lua, 1) == false)
        {
            OutputDebugStringA("incorrect argument\r\n");
            return luaL_error(Lua, "incorrect argument"); // Jumps back -> is like a throw
        }
        lua_Integer iEvtNr = lua_tointeger(Lua, 1);

        lua_getglobal(Lua, "__this__");
        if (!lua_islightuserdata(Lua, -1))
        {
            OutputDebugStringA("internel error\r\n");
            return luaL_error(Lua, "internel error"); // Jumps back -> is like a throw
        }
        LuaScript* pThis = reinterpret_cast<LuaScript*>(lua_touserdata(Lua, -1));

        std::lock_guard<std::mutex> lock(s_mxThrLck);
        for (EventList::iterator itEvent = begin(pThis->m_ummCbFunc); itEvent != end(pThis->m_ummCbFunc); ++itEvent)
        {
            if (itEvent->first == iEvtNr)
            {
                luaL_unref(Lua, LUA_REGISTRYINDEX, itEvent->second.second);
                pThis->m_ummCbFunc.erase(itEvent);
                break;
            }
        }
        lua_pop(Lua, 1);

        return 0;
    });
    lua_setglobal(m_Lua, "UnRegisterEventNotify");
}

LuaScript::~LuaScript()
{
    for (auto& itEvent : m_ummCbFunc)
        luaL_unref(m_Lua, LUA_REGISTRYINDEX, itEvent.second.second);
    m_ummCbFunc.clear();

    if (lua_gettop(m_Lua) != 0)
        OutputDebugStringA(std::string("Lua stack error, stack > 0: " + std::to_string(lua_gettop(m_Lua)) + "\r\n").c_str());

    lua_close(m_Lua);
    s_mxLock.unlock();
}

bool LuaScript::LoadScriptFile(const std::string& strScriptFileName)
{
    if (luaL_loadfile(m_Lua, &strScriptFileName[0]) == LUA_OK)
    {
        if (lua_pcall(m_Lua, 0, 1, 0) == LUA_OK)    // Executes the script
        {
            lua_pop(m_Lua, lua_gettop(m_Lua));      // removes the return element from the Stack
            m_bScriptLoaded = true;
            return m_bScriptLoaded;
        }
    }

    s_strLuaErr = lua_tostring(m_Lua, -1);
    // Pop the error value
    lua_pop(m_Lua, 1);

    return m_bScriptLoaded;
}

bool LuaScript::CallFunction(const std::string& strFnName, ScrPara_t InPara, int nExpectedRetParam, ScrPara_t& OutPara)
{
    bool bRet = false;

    lua_State* LThread = GetThreadState();

    lua_getglobal(LThread, &strFnName[0]);
    if (lua_isfunction(LThread, -1))
    {
        int iInParam = PushParameter(LThread, InPara);

        // Execute function with arguments and return value
        if (lua_pcall(LThread, iInParam, nExpectedRetParam, 0) == LUA_OK)
        {
            PopParameter(LThread, OutPara);
            bRet = true;
        }
        else
        {
            s_strLuaErr = lua_tostring(LThread, -1);
            // Pop the error value
            lua_pop(LThread, 1);
        }
    }
    else
        s_strLuaErr = "Not a lua function";

    ReleaseThreadState(LThread);

    return bRet;
}

bool LuaScript::NotifyEvent(const std::string& strEvent, ScrPara_t InPara, int nExpectedRetParam, ScrPara_t& OutPara)
{
    bool bRet = true;

    lua_State* LThread = GetThreadState();

    for (auto& itEvent : m_ummCbFunc)
    {
        if (itEvent.second.first != strEvent)
            continue;

        lua_rawgeti(LThread, LUA_REGISTRYINDEX, itEvent.second.second);
        int nLeaveOnStack = lua_gettop(LThread);
        lua_pushvalue(LThread, 1);    // Dupplicate value to re ref it again
        nLeaveOnStack = lua_gettop(LThread) - nLeaveOnStack;

        int iInParam = PushParameter(LThread, InPara);
        if (lua_pcall(LThread, iInParam, nExpectedRetParam, 0) == LUA_OK)
            PopParameter(LThread, OutPara, nLeaveOnStack);
        else
        {
            s_strLuaErr = lua_tostring(LThread, -1);
            // Pop the error value
            lua_pop(LThread, 1);

            itEvent.second.second = luaL_ref(LThread, LUA_REGISTRYINDEX);

            bRet = false;
            break;
        }

        itEvent.second.second = luaL_ref(LThread, LUA_REGISTRYINDEX);
    }

    ReleaseThreadState(LThread);

    return bRet;
}

void LuaScript::RegisterFunction(std::string strFnName, FuncReg_t fnRegistrier)
{
    lua_pushinteger(m_Lua, ++m_NextRegFnNr);
    lua_pushlightuserdata(m_Lua, this);
    lua_pushcclosure(m_Lua, [](lua_State *Lua) -> int
    {
        int iFnId = lua_upvalueindex(2);
        LuaScript* pThis = reinterpret_cast<LuaScript*>(lua_touserdata(Lua, iFnId));
        iFnId = lua_upvalueindex(1);
        lua_Integer nFnId = lua_tointeger(Lua, iFnId);

        auto itRegFunc = pThis->m_mRegFunctions.find(nFnId);
        if (itRegFunc == end(pThis->m_mRegFunctions))
        {
            OutputDebugStringA("incorrect argument\r\n");
            return luaL_error(Lua, "incorrect argument"); // Jumps back -> is like a throw
        }

        ScrPara_t Para;
        pThis->PopParameter(Lua, Para);
        ScrPara_t RetPara = itRegFunc->second(Para);
        int iRetParam = pThis->PushParameter(Lua, RetPara);
        return iRetParam;
    }, 2);

    // Get the value on top of the stack
    // and set as a global, in this case is the function
    lua_setglobal(m_Lua, &strFnName[0]);

    m_mRegFunctions.emplace(m_NextRegFnNr, fnRegistrier);
}

ScrPara_t LuaScript::GetGlobalValue(const std::string& strVarname)
{
    lua_getglobal(m_Lua, &strVarname[0]);
    ScrPara_t OutPara;
    PopParameter(m_Lua, OutPara);

    return OutPara;
}

bool LuaScript::SetGlobalValue(const std::string& strVarname, const ScrPara_t& Value)
{
    int iInParam = PushParameter(m_Lua, Value);
    if (iInParam == 1)
    {
        lua_setglobal(m_Lua, &strVarname[0]);
        return true;
    }
    return false;
}

std::string LuaScript::GetErrorString()
{
    return s_strLuaErr;
}

void LuaScript::LuaLock(lua_State* /*L*/)
{
    s_mxLock.lock();
}

void LuaScript::LuaUnlock(lua_State* /*L*/)
{
    s_mxLock.unlock();
}

lua_State* LuaScript::GetThreadState()
{
    lua_State* LThread = m_Lua;
    if (std::this_thread::get_id() != m_ThreadId)
    {
        std::lock_guard<std::mutex> lock(s_mxThrLck);
        LThread = lua_newthread(m_Lua);
    }
    return LThread;
}

void LuaScript::ReleaseThreadState(lua_State* LThread)
{
    if (LThread != m_Lua)
    {
        std::lock_guard<std::mutex> lock(s_mxThrLck);
        int iStSize = lua_gettop(m_Lua);
        for (int i = 1; i <= iStSize; ++i)
        {
            if (lua_tothread(m_Lua, i) == LThread)
            {
                lua_remove(m_Lua, i);
                break;
            }
        }
    }
}

int LuaScript::PushParameter(lua_State* LThread, const ScrPara_t& InPara)
{
    int iInParam = 0;
    std::function<void(ScrPara_t)> fnPushParameter = [&LThread, &iInParam, &fnPushParameter](const ScrPara_t Param)
    {
        if (std::holds_alternative<bool>(Param))
            lua_pushboolean(LThread, std::get<bool>(Param)) ,++iInParam;
        else if (std::holds_alternative<int>(Param))
            lua_pushinteger(LThread, std::get<int>(Param)) ,++iInParam;
        else if (std::holds_alternative<double>(Param))
            lua_pushnumber(LThread, std::get<double>(Param)) ,++iInParam;
        else if (std::holds_alternative<std::string>(Param))
        {
            std::string strTmp = std::get<std::string>(Param);
            lua_pushlstring(LThread, &strTmp[0], strTmp.size()) ,++iInParam;
        }
        else if (std::holds_alternative<void*>(Param))
            lua_pushlightuserdata(LThread, std::get<void*>(Param)) ,++iInParam;
        else if (std::holds_alternative<ParaTable_t>(Param))
        {
            ParaTable_t tblParam = std::get<ParaTable_t>(Param);

            lua_newtable(LThread);        // table
            int iRow = 0;
            for (auto& tblRow : tblParam)
            {
                lua_newtable(LThread);    // table row
                lua_pushstring(LThread, &tblRow.first[0]);    // Col 1
                lua_rawseti(LThread,-2,1);
                for (size_t n = 0; n < tblRow.second.size(); ++n)
                {
                    lua_pushstring(LThread, &tblRow.second[n][0]);// Col 2
                    lua_rawseti(LThread,-2,2 + static_cast<int>(n));
                }
                lua_rawseti(LThread,-2, ++iRow);
            }
            ++iInParam;
        }
        else if (std::holds_alternative<std::vector<ScrPara_t>>(Param))
        {
            std::vector<ScrPara_t> vecParam = std::get<std::vector<ScrPara_t>>(Param);
            for(auto& var : vecParam)
                fnPushParameter(var);
        }
        else
            static_assert(true, "parameter typ not suported!");

        /*std::visit([&LThread, &iInParam, fnPushParameter](auto&& arg)
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, bool>)
                lua_pushboolean(LThread, arg) ,++iInParam;
            else if constexpr (std::is_same_v<T, int>)
                lua_pushinteger(LThread, arg) ,++iInParam;
            else if constexpr (std::is_same_v<T, double>)
                lua_pushnumber(LThread, arg) ,++iInParam;
            else if constexpr (std::is_same_v<T, std::string>)
                lua_pushlstring(LThread, &arg[0], arg.size()) ,++iInParam;
            else if constexpr (std::is_same_v<T, void*>)
                lua_pushlightuserdata(LThread, arg) ,++iInParam;
            else if constexpr (std::is_same_v<T, ParaTable_t>)
            {
                lua_newtable(LThread);        // table
                int iRow = 0;
                for (auto& tblRow : arg)
                {
                    lua_newtable(LThread);    // table row
                    lua_pushstring(LThread, &tblRow.first[0]);    // Col 1
                    lua_rawseti(LThread,-2,1);
                    for (int n = 0; n < tblRow.second.size(); ++n)
                    {
                        lua_pushstring(LThread, &tblRow.second[n][0]);// Col 2
                        lua_rawseti(LThread,-2,2 + n);
                    }
                    lua_rawseti(LThread,-2, ++iRow);
                }
                ++iInParam;
            }
            else if constexpr (std::is_same_v<T, std::vector<ScrPara_t>>)
            {
                for(auto& var : arg)
                    fnPushParameter(var);
            }
            else
                static_assert(always_false_v<T>, "parameter typ not suported!");
        }, Param);*/
    };
    fnPushParameter(InPara);

    return iInParam;
}

void LuaScript::PopParameter(lua_State* LThread, ScrPara_t& OutPara, int nLeaveOnStack/* = 0*/)
{
    int nCountRetParam = lua_gettop(LThread);
    for (int n = 0; n < nCountRetParam - nLeaveOnStack; ++n)
    {
        if (lua_isboolean(LThread, -1))
            OutPara = (lua_toboolean(LThread, -1) == 1 ? true : false);
        else if (lua_isinteger(LThread, -1))
            OutPara = static_cast<int>(lua_tointeger(LThread, -1));
        else if (lua_isnumber(LThread, -1))
            OutPara = lua_tonumber(LThread, -1);
        else if (lua_isstring(LThread, -1))
            OutPara = std::string(lua_tostring(LThread, -1));
        else if (lua_isuserdata(LThread, -1))
            OutPara = lua_touserdata(LThread, -1);
        else if (lua_istable(LThread, -1))
        {
            std::vector<ScrPara_t> tblReturn;
            lua_pushnil(LThread);
            while(lua_next(LThread, -2) != 0)
            {
                std::vector<ScrPara_t> tblRow;
                tblRow.push_back(std::string(lua_tostring(LThread, -2)));
                if(lua_isstring(LThread, -1))
                    tblRow.push_back(std::string(lua_tostring(LThread, -1)));
                else if(lua_isnumber(LThread, -1))
                    tblRow.push_back(lua_tonumber(LThread, -1));
                lua_pop(LThread, 1);
                tblReturn.push_back(tblRow);
            }
            OutPara = tblReturn;
        }
        lua_pop(LThread, 1);
    }
}
