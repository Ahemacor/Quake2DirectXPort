#pragma once

#ifdef __cplusplus
#undef CINTERFACE
#endif

#include <Windows.h>
#include <string>

void ExitOnFailure(const HRESULT& hr, const std::string& line);

#define ENSURE_RESULT(HR) (ExitOnFailure(HR, __FUNCTION__))