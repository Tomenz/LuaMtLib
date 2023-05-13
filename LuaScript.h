
#include <thread>
#include <mutex>
#include <variant>
#include <vector>
#include <unordered_map>
#include <functional>

extern "C" {
// you have to define LUA_USER_H="lualock.h" to compile the lua source, so it uses the lock / unlock callbacks für the mutex
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

using ParaTable_t = std::unordered_map<std::string, std::vector<std::string>>;
template< typename T >
using Var = std::variant<bool, int, double, std::string, void*, ParaTable_t, std::vector<T>>;
// tie the knot
template <template<class> class K>
struct Fix : K<Fix<K>>
{
   using K<Fix>::K;
};
using ScrPara_t = Fix<Var>;
template<class> inline constexpr bool always_false_v = false;

using EventList = std::unordered_map<lua_Integer, std::pair<std::string, int>>;
using FuncReg_t = std::function<ScrPara_t(ScrPara_t)>;

class LuaScript
{
public:
    LuaScript();
    virtual ~LuaScript();

    bool LoadScriptFile(const std::string& strScriptFileName);
    bool CallFunction(const std::string& strFnName, ScrPara_t InPara, int nExpectedRetParam, ScrPara_t& OutPara);
    bool NotifyEvent(const std::string& strEvent, ScrPara_t InPara, int nExpectedRetParam, ScrPara_t& OutPara);
    void RegisterFunction(std::string strFnName, FuncReg_t fnRegistrier);
    ScrPara_t GetGlobalValue(const std::string& strVarname);
    bool SetGlobalValue(const std::string& strVarname, const ScrPara_t& Value);

    std::string GetErrorString();
    static void LuaLock(lua_State* L);
    static void LuaUnlock(lua_State* L);

    bool IsSLoaded() { return m_bScriptLoaded; }

private:
    lua_State* GetThreadState();
    void ReleaseThreadState(lua_State* LThread);
    int PushParameter(lua_State* LThread, const ScrPara_t& InPara);
    void PopParameter(lua_State* LThread, ScrPara_t& OutPara, int nLeaveOnStack = 0);

private:
    std::thread::id     m_ThreadId;
    lua_State*          m_Lua;
    bool                m_bScriptLoaded;
    static std::mutex   s_mxLock;
    static std::mutex   s_mxThrLck;
    thread_local static std::string s_strLuaErr;
    EventList           m_ummCbFunc;
    lua_Integer         m_NextEvtNr;
    std::unordered_map<lua_Integer, FuncReg_t> m_mRegFunctions;
    lua_Integer         m_NextRegFnNr;
};
