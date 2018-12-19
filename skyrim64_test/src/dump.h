#pragma once

void DumpDisableBreakpoint();
void DumpEnableBreakpoint();

DWORD WINAPI DumpWriterThread(LPVOID Arg);
LONG WINAPI DumpExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo);