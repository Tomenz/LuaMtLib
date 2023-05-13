-- Comment

EvtNotifyId = 0
CountWordsCount = 0

function my_function(a, b)
    return a + b
end

function OutTable(tblTest)
    local n=#tblTest
    for i=1,n do
        local m=#tblTest[i]
        local str=""
        for e=1,m do
            str = str .. " " .. tblTest[i][e]
        end
        --OutputDebugString("  " .. tostring(m) .. "\r\n")
        --OutputDebugString("  " .. tblTest[i][1] .. "," .. tblTest[i][2] .. "\r\n")
        OutputDebugString(str .. "\r\n")
    end
end

local function tableHasKey(table,key)
    return table[key] ~= nil
end

function CountWords(strFile, iThreadNr)
    local t = {}

    CountWordsCount = CountWordsCount + 1   -- Dummy inc. the global variable
    DbgOut("Lua loop start " .. tostring(iThreadNr))
    local file = io.open(strFile, "r" )
    if ( io.type( file ) == "file" ) then
        for line in file:lines() do
            if ( line == nil ) then
               break
            end

            for k in string.gmatch(line, "(%w+)") do
                if (tableHasKey(t,k)) then
                    t[k] = t[k] + 1
                else
                    t[k] = 1
                end
                --OutputDebugString("  " .. k .. "\r\n")
            end

        end
        file:close()
    end
    DbgOut("Lua loop end " .. tostring(iThreadNr))
    return t
end

function Init()
    OutputDebugString(os.date() .. " - Init aufgerufen\r\n")
    EvtNotifyId = RegisterEventNotify("EvDoSomething", my_function)
    OutputDebugString("Event Notify ID = " .. tostring(EvtNotifyId) .. "\r\n")
end

function Fini()
    if (EvtNotifyId ~= 0) then
        UnRegisterEventNotify(EvtNotifyId)
        EvtNotifyId = 0
    end
    OutputDebugString(os.date() .. " - Fini aufgerufen\r\n")
end

if (_G["_VERSION_"] == nil) then
    OutputDebugString("_VERSION_ is not defined\r\n")
else
    OutputDebugString("_VERSION_ is " .. tostring(_VERSION_) .. "\r\n")
end