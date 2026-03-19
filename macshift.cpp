/*
 * Macshift - the simple Windows MAC address changing utility
 *
 * Copyright (c) 2004 Nathan True <macshift@natetrue.com>
 * http://www.natetrue.com/
 * Copyright (c) Project Nayuki
 * https://www.nayuki.io/page/macshift-nayukis-version
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <cstddef>
#include <cstdlib>
#include <cwchar>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <windows.h>
#include <winreg.h>
#include <objbase.h>
#include <netcon.h>


 // Function prototypes
static void submain(const std::vector<std::string>& argVec);
static void showHelp(const std::string& exePath);
static std::string formatMac(const std::string& str);
static bool isValidMac(const std::string& str);
static std::string randomMac();
static std::string findAdapterId(const std::string& adapterName);
static void setMac(const std::string& adapterId, const std::string& newMac);
static void resetAdapter(const std::string& AdapterName);
static void setRandomMacState(const std::string& adapterId);
static void clearLease(const std::string& adapterId);
static void coloredText(bool setclear);


int main(int argc, char** argv) {
	std::cerr << "Macshift v2.0 - the simple Windows MAC address changing utility" << std::endl;
	coloredText(true);
	std::cout << "Macshift v2.0-qitamic1.3 https://github.com/qitamic/Macshift" << std::endl;
	coloredText(false);


	std::vector<std::string> argVec;
	for (int i = 0; i < argc; i++)
		argVec.push_back(std::string(argv[i]));

	srand(static_cast<unsigned int>(GetTickCount64()));

	try {
		submain(argVec);
	} catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


static void submain(const std::vector<std::string>& argVec) {
	std::string adapterName = "";
	bool isMacModeSet = false;
	std::string newMac = randomMac();

	// Parse command-line arguments
	for (std::size_t i = 1; i < argVec.size(); i++) {
		const std::string& arg = argVec.at(i);
		if (arg.find("-") == 0) {  // A flag
			if (arg == "-h")
				showHelp(argVec.at(0));
			else if (arg == "-d" || arg == "-r" || arg == "-a") {
				if (isMacModeSet)
					throw std::invalid_argument("Command-line arguments contain more than one MAC address mode");
				isMacModeSet = true;
				if (arg == "-d")
					newMac = "";
				else if (arg == "-r")
					;  // Do nothing else because newMac is already random
				else if (arg == "-a") {
					if (argVec.size() - i <= 1)
						throw std::invalid_argument("Missing MAC address argument");
					i++;
					std::string val = formatMac(argVec.at(i));
					if (!isValidMac(val))
						throw std::invalid_argument("Invalid MAC address, must match pattern /[0-9a-fA-F]{12}/");
					newMac = val;
				} else
					throw std::logic_error("Unreachable");
			} else
				throw std::invalid_argument("Unrecognized command-line flag");
		} else {  // Not a flag
			if (!adapterName.empty())
				throw std::invalid_argument("Command-line arguments contain more than one network adapter name");
			adapterName = arg;
		}
	}

	if (adapterName == "")
		showHelp(argVec.at(0));

	std::cerr << "New MAC address: ";
	if (newMac == "")
		std::cerr << "(restore)";
	else {
		for (std::size_t i = 0; i < 12; i += 2) {
			if (i > 0)
				std::cerr << "-";
			std::cerr << newMac.substr(i, 2);
		}
	}
	std::cerr << std::endl;

	std::string adapterId = findAdapterId(adapterName);
	std::cerr << "Network adapter ID: " << adapterId << std::endl;
	setMac(adapterId, newMac);
	setRandomMacState(adapterId);
	clearLease(adapterId);
	resetAdapter(adapterName);
}


static void showHelp(const std::string& exePath) {
	std::cerr << "https://www.nayuki.io/page/macshift-nayukis-version" << std::endl;
	std::cerr << std::endl;
	std::cerr << "Usage: " << exePath << " AdapterName [Options]" << std::endl;
	std::cerr << std::endl;
	std::cerr << "Example: " << exePath << " \"Wi-Fi\" -r" << std::endl;
	std::cerr << "Example: " << exePath << " \"Ethernet\" -a 02ABCDEF9876" << std::endl;

	const std::vector<const char*> LINES{
		"",
		"Options:",
		"    -r               Use a random MAC address (default action).",
		"    -a MacAddress    Use the given MAC address.",
		"    -d               Restore the original MAC address.",
		"",
		"    -h               Show this help screen.",
		"",
		"Macshift uses special undocumented functions in the Windows COM Interface",
		"that allow you to change an adapter's MAC address without needing to restart.",
		"",
		"When you change a MAC address, all your connections",
		"are closed automatically and your adapter is reset.",
	};
	for (const char* line : LINES)
		std::cerr << line << std::endl;

	std::cerr << std::endl;
	coloredText(true);	
	std::cerr << "RandomMacState" << std::endl;
	coloredText(false);
	std::cerr << "Computer\\HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\WlanSvc\\Interfaces\\" << std::endl;
	coloredText(true);
	std::cerr << "DhcpIPAddress DhcpSubnetMask DhcpServer DhcpDefaultGateway DhcpNameServer" << std::endl;
	coloredText(false);
	std::cerr << "Computer\\HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces" << std::endl;
	coloredText(true);
	std::cerr << "NetworkAddress" << std::endl;
	coloredText(false);
	std::cerr << "Computer\\HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}\\" << std::endl;
	
	std::exit(EXIT_FAILURE);
}


static std::string formatMac(const std::string& str) {
	std::string s = "";
	for (int i = 0; i < str.size(); i++) {
		char c = str.at(i);
		if (c != ':' && c != '-')
			s += c;
	}
	return s;
}


static bool isValidMac(const std::string& str) {
	if (str.size() != 12)
		return false;
	for (int i = 0; i < 12; i++) {
		char c = str.at(i);
		bool ok = ('0' <= c && c <= '9')
			|| ('a' <= c && c <= 'f')
			|| ('A' <= c && c <= 'F');
		if (!ok)
			return false;
	}
	return true;
}


static std::string randomMac() {
	long long temp = 0;
	for (int i = 0; i < 6; i++) {
		int b = rand() & 0xFF;
		if (i == 0)
			b = (b & 0xFC) | 0x02;  // Set local and unicast bits
		temp = (temp << 8) | b;
	}

	static const std::string HEX_DIGITS = "0123456789ABCDEF";
	std::string result;
	for (int i = 11; i >= 0; i--) {
		result.insert(result.begin(), HEX_DIGITS.at(temp & 0xF));
		temp >>= 4;
	}
	return result;
}


// https://stackoverflow.com/questions/161177/does-c-support-finally-blocks-and-whats-this-raii-i-keep-hearing-about/25510879#25510879
template <typename F>
class Finally {
private: F func;
public: Finally(F f) :
	func(f) {}
public: ~Finally() {
	func();
}
};
template <typename F>
static Finally<F> finally(F f) {
	return Finally<F>(f);
}


static constexpr std::size_t TEXT_BUFFER_LEN = 512;


static std::string findAdapterId(const std::string& adapterName) {
	HKEY hListKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}", 0, KEY_READ, &hListKey) != ERROR_SUCCESS)
		throw std::runtime_error("Failed to open adapter list key");
	auto hListKeyFinally = finally([hListKey] { RegCloseKey(hListKey); });

	for (DWORD i = 0; ; i++) {
		std::string nameStr;
		{
			std::vector<char> name(TEXT_BUFFER_LEN);
			DWORD nameLen = static_cast<DWORD>(name.size());
			LSTATUS stat = RegEnumKeyEx(hListKey, i, name.data(), &nameLen, 0, nullptr, nullptr, nullptr);
			if (stat == ERROR_NO_MORE_ITEMS)
				break;
			if (stat != ERROR_SUCCESS)
				throw std::runtime_error("Failed to enumerate registry keys");
			nameStr = name.data();
		}

		std::string subkey = nameStr + "\\Connection";
		HKEY hKey;
		if (RegOpenKeyEx(hListKey, subkey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
			continue;
		auto hKeyFinally = finally([hKey] { RegCloseKey(hKey); });

		std::vector<char> value(TEXT_BUFFER_LEN);
		DWORD valueLen = static_cast<DWORD>(value.size());
		if (RegQueryValueEx(hKey, "Name", nullptr, nullptr, reinterpret_cast<LPBYTE>(value.data()), &valueLen) == ERROR_SUCCESS
			&& std::string(value.data()) == adapterName) {
			return nameStr;
		}
	}
	throw std::runtime_error("Failed to find an adapter with the given name; please recheck your Network Connections");
}


static void setMac(const std::string& adapterId, const std::string& newMac) {
	HKEY hListKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}", 0, KEY_READ, &hListKey) != ERROR_SUCCESS)
		throw std::runtime_error("Failed to open adapter list key in Phase 2");
	auto hListKeyFinally = finally([hListKey] { RegCloseKey(hListKey); });

	for (DWORD i = 0; ; i++) {
		std::vector<char> name(TEXT_BUFFER_LEN);
		{
			DWORD nameLen = static_cast<DWORD>(name.size());
			LSTATUS stat = RegEnumKeyEx(hListKey, i, name.data(), &nameLen, 0, nullptr, nullptr, nullptr);
			if (stat == ERROR_NO_MORE_ITEMS)
				break;
			if (stat != ERROR_SUCCESS)
				throw std::runtime_error("Failed to enumerate registry keys");
		}

		HKEY hKey;
		if (RegOpenKeyEx(hListKey, name.data(), 0, KEY_READ | KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
			continue;
		auto hKeyFinally = finally([hKey] { RegCloseKey(hKey); });

		std::vector<char> value(TEXT_BUFFER_LEN);
		DWORD valueLen = static_cast<DWORD>(value.size());
		if (RegQueryValueEx(hKey, "NetCfgInstanceId", nullptr, nullptr, reinterpret_cast<LPBYTE>(value.data()), &valueLen) == ERROR_SUCCESS
			&& std::string(value.data()) == adapterId) {
			if (RegSetValueEx(hKey, "NetworkAddress", 0, REG_SZ, reinterpret_cast<const BYTE*>(newMac.c_str()), static_cast<DWORD>(newMac.size() + 1)) != ERROR_SUCCESS)
				throw std::runtime_error("Failed write NetworkAddress");
			std::cerr << "Wrote NetworkAddress" << std::endl;
			return;  // Success
		}
	}
	throw std::runtime_error("Failed to find adapter by ID; please run this program as Administrator");
}
static void setRandomMacState(const std::string& adapterId){
	coloredText(true);
	std::cout << "Write RandomMacState... ";
	std::string reg = "SOFTWARE\\Microsoft\\WlanSvc\\Interfaces\\" + adapterId;
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, reg.c_str(), 0, KEY_SET_VALUE | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS) {
		byte data[] = { 01, 00, 00, 00 };
		if (RegSetValueEx(hKey, "RandomMacState", 0, REG_BINARY, (LPBYTE)data, sizeof(data) / sizeof(data[0])) == ERROR_SUCCESS)
			std::cout << "Done" << std::endl;
		else
			throw std::runtime_error("Failed"); //will it come here?
	}
	else {
		std::cout << "Failed (not a Wi-Fi adapter?)" << std::endl;
	}
	auto hListKeyFinally = finally([hKey] { RegCloseKey(hKey); });	
	coloredText(false);
}
static void clearLease(const std::string& adapterId)
{
	coloredText(true);
	std::cout << "Clear DHCP lease... ";

	// Path to the specific interface in the registry
	std::string subKey = "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\" + adapterId;

	HKEY hKey;
	// Open the key with SET_VALUE permissions to allow deletion
	LSTATUS status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_SET_VALUE, &hKey);

	if (status == ERROR_SUCCESS) {
		// List of DHCP values to clear
		std::vector<std::string> valuesToDelete = {
			"DhcpIPAddress",
			"DhcpSubnetMask",
			"DhcpServer",
			"DhcpDefaultGateway",
			"DhcpNameServer"
		};

		bool error = false;
		LSTATUS delStatus;
		for (const auto& value : valuesToDelete) {
			delStatus = RegDeleteValueA(hKey, value.c_str());
			if (delStatus == ERROR_SUCCESS) {
			}
			else if (delStatus != ERROR_FILE_NOT_FOUND) {
				error = true;
			}
		}
		if(error)
			std::cerr << "Failed! (" << delStatus << ")" << std::endl;
		else
			std::cout << "Done" << std::endl;

		coloredText(false);
		RegCloseKey(hKey);
	}
	else {
		std::cerr << "Could not open registry key. Ensure you are running as Admin. Error: " << status << std::endl;
	}
	
}
static void resetAdapter(const std::string& adapterName) {
	HMODULE netshellLib = LoadLibrary("Netshell.dll");
	if (netshellLib == nullptr)
		throw std::runtime_error("Failed to load Netshell.dll");
	auto netshellLibFinally = finally([netshellLib] { FreeLibrary(netshellLib); });

	auto NcFreeNetConProperties = reinterpret_cast<void(__stdcall*)(struct tagNETCON_PROPERTIES*)>(
		GetProcAddress(netshellLib, "NcFreeNetconProperties"));
	if (NcFreeNetConProperties == nullptr)
		throw std::runtime_error("Failed to load function from DLL");

	std::wstring buf;
	for (std::size_t i = 0; i < adapterName.size(); i++)
		buf.push_back(static_cast<wchar_t>(adapterName.at(i)));

	(void)CoInitialize(nullptr);
	auto comFinally = finally([] { CoUninitialize(); });

	INetConnectionManager* conMgr;
	struct _GUID guid = { 0xBA126AD1, 0x2166, 0x11D1, {0xB1,0xD0,0x00,0x80,0x5F,0xC1,0x27,0x0E} };
	if (::CoCreateInstance(guid, nullptr, CLSCTX_ALL, __uuidof(INetConnectionManager), reinterpret_cast<void**>(&conMgr)) != S_OK)
		throw std::runtime_error("Failed to create connection manager");
	auto conMgrFinally = finally([conMgr] { conMgr->Release(); });

	IEnumNetConnection* enumCon;
	conMgr->EnumConnections(NCME_DEFAULT, &enumCon);
	if (enumCon == nullptr)
		throw std::runtime_error("Could not enumerate Network Connections");
	auto enumConFinally = finally([enumCon] { enumCon->Release(); });

	while (true) {
		INetConnection* netCon;
		{
			ULONG fetched;
			enumCon->Next(1, &netCon, &fetched);
			if (fetched == 0)
				break;
		}
		if (netCon == nullptr)
			continue;

		NETCON_PROPERTIES* conProp;
		netCon->GetProperties(&conProp);
		if (conProp == nullptr)
			continue;
		auto conPropFinally = finally([conProp, NcFreeNetConProperties] { NcFreeNetConProperties(conProp); });

		if (std::wcscmp(conProp->pszwName, buf.c_str()) == 0) {
			std::cerr << "Resetting adapter" << std::endl;
			netCon->Disconnect();
			netCon->Connect();
			return;  // Success
		}
	}
	throw std::runtime_error("Failed to find adapter by name");
}

static DWORD colorOriginalOut, colorOriginalErr;
static void coloredText(bool setclear)
{
	HANDLE hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hConsoleErr = GetStdHandle(STD_ERROR_HANDLE);
	if (setclear){
		CONSOLE_SCREEN_BUFFER_INFO InfoOut, InfoErr;
		GetConsoleScreenBufferInfo(hConsoleOut, &InfoOut); //original
		GetConsoleScreenBufferInfo(hConsoleErr, &InfoErr); //original
		colorOriginalOut = InfoOut.wAttributes;
		colorOriginalErr = InfoErr.wAttributes;
		https://stackoverflow.com/questions/4053837/colorizing-text-in-the-console-with-c
		SetConsoleTextAttribute(hConsoleOut, 14);
		SetConsoleTextAttribute(hConsoleErr, 14);
	} else{
		SetConsoleTextAttribute(hConsoleOut, colorOriginalOut);
		SetConsoleTextAttribute(hConsoleErr, colorOriginalErr);
	}	
}

