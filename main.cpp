#include <Windows.h>

#include <cstdlib>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>

using namespace std::literals;

namespace
{
constexpr auto filenameWithPATH = L"_path.txt";
constexpr auto wrappedExeSuffix = L"_real";
constexpr auto envVarPATH = L"PATH";

std::pair<std::wstring, std::wstring> programFileInfo()
{
	std::wstring programFullPath(MAX_PATH, L'\0');
	const auto bufSize = GetModuleFileNameW(nullptr, programFullPath.data(), static_cast<DWORD>(programFullPath.size()));
	programFullPath.resize(bufSize);

	const auto afterLastSlash = programFullPath.find_last_of(L"\\/") + 1;
	const auto dotPos = programFullPath.find_last_of(L'.');

	return std::make_pair(programFullPath.substr(0, afterLastSlash), programFullPath.substr(afterLastSlash, dotPos - afterLastSlash));
}

std::wstring programCommandLineArgs()
{
	const std::wstring_view programCommandLine = GetCommandLineW();

	// the called program (i.e. argv[0]) may be enclosed in quotes
	const auto firstArgDelimiter = programCommandLine[0] == L'"' ? L"\" "sv : L" "sv;
	const auto firstArgDelimiterPos = programCommandLine.find(firstArgDelimiter);
	if (firstArgDelimiterPos == std::wstring::npos)
		return {};

	// include the leading space
	const auto beforeSpaceDelimiter = firstArgDelimiterPos + firstArgDelimiter.size() - 1;
	return std::wstring{programCommandLine.substr(beforeSpaceDelimiter)};
}

std::wstring readFirstLineFromFile(const std::wstring& filePath)
{
	std::wifstream file{filePath};
	if (!file.is_open())
		return {};

	std::wstring line;
	std::getline(file, line);
	return line;
}

void modifyPATHEnvVar(std::wstring&& valueToPrepend)
{
	if (valueToPrepend.empty())
		return;

	std::wstring envVarValue;
	const auto bufSize = GetEnvironmentVariableW(envVarPATH, nullptr, 0);
	if (bufSize > 0)
	{
		envVarValue.resize(bufSize);
		GetEnvironmentVariableW(envVarPATH, envVarValue.data(), static_cast<DWORD>(envVarValue.size()));
		// the function also fills the trailing null character, strip it
		envVarValue.pop_back();
	}

	envVarValue.reserve(bufSize + valueToPrepend.size());
	envVarValue.insert(0, std::move(valueToPrepend));
	SetEnvironmentVariableW(envVarPATH, envVarValue.c_str());
}

DWORD startExecutable(std::wstring&& exeCommandLine)
{
	std::wstring commandLine = std::move(exeCommandLine);

	STARTUPINFOW startupInfo{};
	startupInfo.cb = sizeof startupInfo;
	startupInfo.dwFlags = STARTF_USESTDHANDLES;
	startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	startupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	startupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);

	PROCESS_INFORMATION processInfo{};
	const auto createProcessResult = CreateProcessW(nullptr, commandLine.data(),
		nullptr, nullptr,
		TRUE, CREATE_UNICODE_ENVIRONMENT,
		nullptr, nullptr,
		&startupInfo, &processInfo);
	if (!createProcessResult)
		return EXIT_FAILURE;

	DWORD exitCode = EXIT_FAILURE;
	if (WaitForSingleObject(processInfo.hProcess, INFINITE) == WAIT_OBJECT_0)
		GetExitCodeProcess(processInfo.hProcess, &exitCode);

	CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);
	for (auto handle : {startupInfo.hStdInput, startupInfo.hStdOutput, startupInfo.hStdError})
		if (handle != nullptr)
			CloseHandle(handle);

	return exitCode;
}
}

int main()
{
	const auto [programDir, programName] = programFileInfo();
	modifyPATHEnvVar(readFirstLineFromFile(programDir + filenameWithPATH));
	return startExecutable(programName + wrappedExeSuffix + programCommandLineArgs());
}
