#include "stdafx.h"
#include "TestLauncherMC.h"

#define CURL_STATICLIB
#define no_init_all deprecated
#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>
#include <string>
#include <locale>
#include <codecvt>
#include <sstream>
#include <algorithm>
#include <curl/curl.h>
#include <json.hpp>
#include <Lmcons.h>
#include <sys/stat.h>

using json = nlohmann::json;

namespace
{
	std::size_t callback(
		const char* in,
		std::size_t size,
		std::size_t num,
		std::string* out)
	{
		const std::size_t totalBytes(size * num);
		out->append(in, totalBytes);
		return totalBytes;
	}
}

std::string join(const std::vector<std::string> & sequence, const std::string & separator)
{
	std::string result;
	for (size_t i = 0; i < sequence.size(); ++i)
		result += sequence[i] + ((i != sequence.size() - 1) ? separator : "");
	return result;
}

std::vector<std::string> split(std::string s, std::string del = " ")
{
	int start = 0;
	int end = s.find(del);
	std::vector<std::string> list;
	while (end != -1) {
		list.push_back(s.substr(start, end - start));
		start = end + del.size();
		end = s.find(del, start);
	}
	list.push_back(s.substr(start, end - start));
	return list;
}

bool IsPathExist(const std::string &s)
{
	struct stat buffer;
	return (stat(s.c_str(), &buffer) == 0);
}

std::wstring version;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	// Register the window class.
	const wchar_t CLASS_NAME[] = L"Sample Window Class";

	TCHAR Versions[2][5] = { TEXT("1.12"), TEXT("1.11") };

	TCHAR A[16];
	int  k = 0;

	WNDCLASS wc = {};

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	// Create the window.

	HWND hwnd = CreateWindowEx(
		0,                              // Optional window styles.
		CLASS_NAME,                     // Window class
		L"Test",    // Window text
		WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,            // Window style

										// Size and position
		CW_USEDEFAULT, CW_USEDEFAULT, 840, 540,

		NULL,       // Parent window    
		NULL,       // Menu
		hInstance,  // Instance handle
		NULL        // Additional application data
	);

	if (hwnd == NULL)
	{
		return 0;
	}

	HWND hWndEditBox = CreateWindow(_T("EDIT"), _T("Player"), WS_BORDER | WS_CHILD | WS_VISIBLE, 500, 450, 100, 20, hwnd, (HMENU)2, hInstance, 0);

	CreateWindow(_T("BUTTON"), _T("PLAY"), WS_CHILD | WS_VISIBLE, 650, 450, 80, 20, hwnd, (HMENU)1, hInstance, 0);

	HWND hWndComboBox = CreateWindow(_T("COMBOBOX"), 0, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | CBS_DROPDOWN | CBS_HASSTRINGS | WS_VSCROLL, 400, 450, 80, 200, hwnd, 0, hInstance, 0);

	memset(&A, 0, sizeof(A));
	for (k = 0; k <= 1; k += 1)
	{
		wcscpy_s(A, sizeof(A) / sizeof(TCHAR), (TCHAR*)Versions[k]);
		
		SendMessage(hWndComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)A);
	}

	SendMessage(hWndComboBox, CB_SETCURSEL, (WPARAM)2, (LPARAM)0);

	ShowWindow(hwnd, nCmdShow);

	// Run the message loop.

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	bool wasHandled = false;
	LRESULT result = 0;

	curl_global_init(CURL_GLOBAL_ALL);

	HDC         hdc;
	PAINTSTRUCT ps;

	switch (uMsg)
	{
	case WM_CREATE:

		return 0;

	case WM_COMMAND:

		if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			int ItemIndex = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL,
				(WPARAM)0, (LPARAM)0);
			TCHAR ListItem[256];
			(TCHAR)SendMessage((HWND)lParam, (UINT)CB_GETLBTEXT,
				(WPARAM)ItemIndex, (LPARAM)ListItem);
			// MessageBox(hwnd, (LPCWSTR)ListItem, TEXT("Item Selected"), MB_OK);
			version = (&ListItem[0]);
			wasHandled = true;
			result = 0;
		}
		else
		switch (LOWORD(wParam))

			case 1:
		{
			TCHAR usernameraw[UNLEN + 1];
			DWORD usernameraw_len = UNLEN + 1;
			GetUserName(usernameraw, &usernameraw_len);
			std::wstring usernametmp(&usernameraw[0]);
			std::string username(usernametmp.begin(), usernametmp.end());

			HWND editBox = GetDlgItem(hwnd, 2);

			int cTxtLen = GetWindowTextLength(editBox);

			std::string nick;

			bool download = false;

			nick.reserve(cTxtLen);

			GetWindowTextA(editBox, const_cast<char*>(nick.c_str()), nick.capacity());

			using convert_type = std::codecvt_utf8<wchar_t>;
			std::wstring_convert<convert_type, wchar_t> converter;
			std::string ver = converter.to_bytes(version);

			int len = ver.length();

			if (len < 1)
			{
				MessageBox(hwnd, _T("Choose the version"), TEXT("Warning"), MB_OK);
				break;
			}

			AllocConsole();
			freopen("CONOUT$", "r", stdout);

			if (!IsPathExist("C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft"))
			{
				{
					std::string tmp = "mkdir C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft";
					system(tmp.c_str());
				}
				{
					std::string tmp = "mkdir C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries";
					system(tmp.c_str());
				}
				{
					std::string tmp = "mkdir C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\versions";
					system(tmp.c_str());
				}
			}
			if (!IsPathExist("C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\versions\\" + ver))
			{
				{
					std::string tmp = "mkdir C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\versions\\" + ver;
					system(tmp.c_str());
				}
				{
					std::string tmp = "mkdir C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\versions\\" + ver + "\\natives";
					system(tmp.c_str());
				}
				download = true;
			}

			CURL *curl;
			curl = curl_easy_init();

			curl_easy_setopt(curl, CURLOPT_URL, "https://launchermeta.mojang.com/mc/game/version_manifest.json");
			curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
			long httpCode(0);
			std::unique_ptr<std::string> httpData(new std::string());
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpData.get());
			curl_easy_perform(curl);
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

			if (httpCode == 200)
			{
				json j;
				int idnt;
				std::string verid;
				j = json::parse(*httpData.get());
				std::vector<json> ja = j["versions"];
				for (int i = 0; i < ja.size(); i++)
				{
					if (ja[i]["id"] == ver)
					{
						idnt = i;
						break;
					}
				}
				std::string url = ja[idnt]["url"].dump();

				if (url.find('"') != std::string::npos) {
					url.erase(remove(url.begin(), url.end(), '"'), url.end());
				}

				//OutputDebugStringA(url.c_str());

				long httpCode2(0);
				std::unique_ptr<std::string> httpData2(new std::string());

				curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpData2.get());
				curl_easy_perform(curl);
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode2);

				//OutputDebugStringA(std::to_string(httpCode2).c_str());

				if (httpCode2 == 200) {
					json js;
					js = json::parse(*httpData2.get());
					std::string clienturl = js["downloads"]["client"]["url"];
					//OutputDebugStringA(clienturl.c_str());
					std::vector<json> libs = js["libraries"];

					if (download)
					{
						{
							std::string tmp = "curl -o C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\versions\\" + ver + "\\client.jar " + clienturl;
							system(tmp.c_str());
						}
					}

					std::vector<std::string> names;

					for (int i = 0; i < libs.size(); i++)
					{
						std::string_view classifiers = "classifiers";
						std::string_view artifact = "artifact";
						if (libs[i]["downloads"].contains(std::string{ classifiers }) == true && libs[i]["downloads"].contains(std::string{ artifact }) == false)
						{
							std::string link;
							std::string tmp;
							std::string_view nativeswindows = "natives-windows";
							std::string_view nativeswindows64 = "natives-windows-64";
							if (libs[i]["downloads"]["classifiers"].contains(std::string{ nativeswindows }) == true) 
							{
								link = libs[i]["downloads"]["classifiers"]["natives-windows"]["url"];
								tmp = libs[i]["downloads"]["classifiers"]["natives-windows"]["path"];
							}
							else if (libs[i]["downloads"]["classifiers"].contains(std::string{ nativeswindows64 }) == true)
							{
								link = libs[i]["downloads"]["classifiers"]["natives-windows-64"]["url"];
								tmp = libs[i]["downloads"]["classifiers"]["natives-windows-64"]["path"];
							}
							std::vector<std::string> tmp2 = split(tmp, "/");
							std::string name = tmp2[tmp2.size() - 1];
							if (download)
							{
								{
									std::string tmp = "curl -o C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\versions\\" + ver + "\\natives\\" + name + " " + link;
									system(tmp.c_str());
								}
								{
									std::string tmp = "tar -xf C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\versions\\" + ver + "\\natives\\" + name + " -C C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\versions\\" + ver + "\\natives";
									system(tmp.c_str());
								}
							}
							//OutputDebugStringA(name.c_str());
						}
						else {
							std::string_view rules = "rules";
							std::string_view os = "os";
							if (libs[i].contains(std::string{ rules }) == true && libs[i]["rules"][0].contains(std::string{ os }) == true)
							{
								continue;
							}
							else
							{

								std::string tmppath = libs[i]["downloads"]["artifact"]["path"];
								std::vector<std::string> vpath = split(tmppath, "/");
								std::string name = vpath[vpath.size() - 1];
								vpath.pop_back();
								std::string path = join(vpath, "\\");
								std::string url = libs[i]["downloads"]["artifact"]["url"];
								std::vector<std::string> paths = split(path, "\\");
								std::vector<std::string> paths2;
								paths2.push_back("C:");
								paths2.push_back("Users");
								paths2.push_back(username);
								paths2.push_back("AppData");
								paths2.push_back("Roaming");
								paths2.push_back("minecraft");
								paths2.push_back("libraries");

								if (download)
								{
									for (int f = 0; f < paths.size(); f++) {
										/*std::string tmpp = "mkdir " + paths[f];
										system(tmpp.c_str());*/
										//OutputDebugStringA(tmpp.c_str());
										for (int g = 0; g < paths2.size(); g++)
										{
											//std::string tmpp = "move " + paths[f] + " " + join(paths2, "\\");
											std::string tmpp = "mkdir " + join(paths2, "\\") + "\\" + paths[f];
											system(tmpp.c_str());
											//OutputDebugStringA(tmpp.c_str());
											/*if (code != 0)
											{
												std::string tmpp = "rmdir " + paths[f];
												system(tmpp.c_str());
											}*/
										}
										paths2.push_back(paths[f]);
									}
									{
										std::string tmp = "curl -o C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\" + path + "\\" + name + " " + url;
										system(tmp.c_str());
									}
									/*{
										std::string tmp = "curl " + url + " -O " + name;
										system(tmp.c_str());
									}
									{
										std::string tmp = "move " + name + " C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\" + path + "\\" + name;
										system(tmp.c_str());
									}*/
								}
								names.push_back(path + "\\" + name);
							}
						}
					}
					// std::string tempcommandj = "java.exe -Dos.version=10.0 -Djava.library.path=C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\versions\\" + ver + "\\natives -cp C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\com\\mojang\\patchy\\1.3.9\\patchy-1.3.9.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\oshi-project\\oshi-core\\1.1\\oshi-core-1.1.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\net\\java\\dev\\jna\\jna\\4.4.0\\jna-4.4.0.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\net\\java\\dev\\jna\\platform\\3.4.0\\platform-3.4.0.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\com\\ibm\\icu\\icu4j-core-mojang\\51.2\\icu4j-core-mojang-51.2.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\net\\sf\\jopt-simple\\jopt-simple\\5.0.3\\jopt-simple-5.0.3.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\com\\paulscode\\codecjorbis\\20101023\\codecjorbis-20101023.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\com\\paulscode\\codecwav\\20101023\\codecwav-20101023.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\com\\paulscode\\libraryjavasound\\20101123\\libraryjavasound-20101123.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\com\\paulscode\\librarylwjglopenal\\20100824\\librarylwjglopenal-20100824.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\com\\paulscode\\soundsystem\\20120107\\soundsystem-20120107.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\io\\netty\\netty-all\\4.1.9.Final\\netty-all-4.1.9.Final.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\com\\google\\guava\\guava\\21.0\\guava-21.0.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\org\\apache\\commons\\commons-lang3\\3.5\\commons-lang3-3.5.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\commons-io\\commons-io\\2.5\\commons-io-2.5.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\commons-codec\\commons-codec\\1.10\\commons-codec-1.10.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\net\\java\\jinput\\jinput\\2.0.5\\jinput-2.0.5.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\net\\java\\jutils\\jutils\\1.0.0\\jutils-1.0.0.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\com\\google\\code\\gson\\gson\\2.8.0\\gson-2.8.0.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\com\\mojang\\authlib\\1.5.25\\authlib-1.5.25.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\com\\mojang\\realms\\1.10.17\\realms-1.10.17.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\org\\apache\\commons\\commons-compress\\1.8.1\\commons-compress-1.8.1.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\org\\apache\\httpcomponents\\httpclient\\4.3.3\\httpclient-4.3.3.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\commons-logging\\commons-logging\\1.1.3\\commons-logging-1.1.3.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\org\\apache\\httpcomponents\\httpcore\\4.3.2\\httpcore-4.3.2.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\it\\unimi\\dsi\\fastutil\\7.1.0\\fastutil-7.1.0.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\org\\apache\\logging\\log4j\\log4j-api\\2.8.1\\log4j-api-2.8.1.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\org\\apache\\logging\\log4j\\log4j-core\\2.8.1\\log4j-core-2.8.1.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\org\\lwjgl\\lwjgl\\lwjgl\\2.9.4-nightly-20150209\\lwjgl-2.9.4-nightly-20150209.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\org\\lwjgl\\lwjgl\\lwjgl_util\\2.9.4-nightly-20150209\\lwjgl_util-2.9.4-nightly-20150209.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\com\\mojang\\text2speech\\1.10.3\\text2speech-1.10.3.jar;C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\versions\\" + ver + "\\client.jar -Xmx8765M -XX:+UnlockExperimentalVMOptions -XX:+UseG1GC -XX:G1NewSizePercent=20 -XX:G1ReservePercent=20 -XX:MaxGCPauseMillis=50 -XX:G1HeapRegionSize=32M -Dfml.ignoreInvalidMinecraftCertificates=true -Dfml.ignorePatchDiscrepancies=true -Djava.net.preferIPv4Stack=true -Dminecraft.applet.TargetDirectory=C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft -Dlog4j.configurationFile=C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\assets\\log_configs\\client-" + ver + ".xml net.minecraft.client.main.Main --version " + ver + " --username " + nick + " --gameDir C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft --assetsDir C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\assets --assetIndex " + ver + " --uuid 2464b184-fe27-11ec-a78b-f02f74958c02 --accessToken null --userType mojang --versionType release --width 925 --height 530";
					// system(tempcommandj.c_str());
					OutputDebugStringA("1");
					std::vector<std::string> tempjp;
					for (int v = 0; v < names.size(); v++)
					{
						tempjp.push_back("C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\libraries\\" + names[v]);
					}

					std::string cp = join(tempjp, ";");
					OutputDebugStringA(nick.c_str());
					std::string tempcommandj = "java.exe -Dos.version=10.0 -Djava.library.path=C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\versions\\" + ver + "\\natives -cp " + cp + ";C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\versions\\" + ver + "\\client.jar -Xmx2048M -XX:+UnlockExperimentalVMOptions -XX:+UseG1GC -XX:G1NewSizePercent=20 -XX:G1ReservePercent=20 -XX:MaxGCPauseMillis=50 -XX:G1HeapRegionSize=32M -Dfml.ignoreInvalidMinecraftCertificates=true -Dfml.ignorePatchDiscrepancies=true -Djava.net.preferIPv4Stack=true -Dminecraft.applet.TargetDirectory=C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft -Dlog4j.configurationFile=C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\assets\\log_configs\\client-" + ver + ".xml net.minecraft.client.main.Main --version " + ver + " --username " + nick.c_str() + " --gameDir C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft --assetsDir C:\\Users\\" + username + "\\AppData\\Roaming\\minecraft\\assets --assetIndex " + ver + " --uuid 2464b184-fe27-11ec-a78b-f02f74958c02 --accessToken null --userType mojang --versionType release --userProperties={}";
					system(tempcommandj.c_str());
				}
			}

			break;
		}

	case WM_DISPLAYCHANGE:
	{
		InvalidateRect(hwnd, NULL, FALSE);
	}
	wasHandled = true;
	result = 0;
	break;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);



		EndPaint(hwnd, &ps);
		return 0;

	case WM_DESTROY:
	{
		PostQuitMessage(0);
	}
	wasHandled = true;
	result = 1;
	break;
	}

	if (!wasHandled)
	{
		result = DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}
