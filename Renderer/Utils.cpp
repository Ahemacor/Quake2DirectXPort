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
        std::string LastError;
        //Get the error message, if any.
        DWORD errorMessageID = ::GetLastError();
        if (errorMessageID != 0)
        {
            LPSTR messageBuffer = nullptr;
            size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

            LastError = std::string(messageBuffer, size);

            //Free the buffer.
            LocalFree(messageBuffer);
        }
        else
        {
            LastError = "Unknown failure";
        }

        std::string errMsg = line;
        errMsg += ": ";
        errMsg += LastError;
        errMsg += "\n";
        MessageBoxA(NULL, errMsg.c_str(), "Error", MB_ICONERROR);
        OutputDebugStringA(errMsg.c_str());
        exit(-1);
    }
}