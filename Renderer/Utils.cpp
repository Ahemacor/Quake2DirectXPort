#include "Utils.h"
#include <comdef.h>

void ExitOnFailure(const HRESULT& hr, const std::string& line)
{
    if (FAILED(hr))
    {
        _com_error err(hr);
        std::string errMsg = line;
        errMsg += ": ";
        errMsg += err.ErrorMessage();
        errMsg += "\n";
        MessageBoxA(NULL, errMsg.c_str(), "Error", MB_ICONERROR);
        OutputDebugStringA(errMsg.c_str());
        exit(-1);
    }
}

void ExitOnFalse(bool isValid, const std::string& line)
{
    if (isValid == false)
    {
        std::string errMsg = line;
        errMsg += ": ";
        errMsg += "critical failure";
        errMsg += "\n";
        MessageBoxA(NULL, errMsg.c_str(), "Error", MB_ICONERROR);
        OutputDebugStringA(errMsg.c_str());
        exit(-1);
    }
}