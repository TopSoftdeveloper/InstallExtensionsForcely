#define _CRT_SECURE_NO_WARNINGS

#pragma once

#ifndef _UTIL_H__
#define _UTIL_H__

#include <Windows.h>
#include <iostream>

BOOL GetParameter();
bool CreateDirectoryRecursively(const std::string& path);
void RunPowerShellCommand(const char* command);
void CopyAllFiles(char* basepath);
BOOL MakeSchedule(std::string time, char* exepath);

#endif
