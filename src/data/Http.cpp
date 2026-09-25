// Contains source code for the Http.cpp file
#include "data/Http.h"

#include <windows.h>
#include <winhttp.h>

namespace
{
    struct ParsedUrl
    {
        std::wstring host;
        std::wstring path;
        INTERNET_PORT port;
        bool secure;
    };

    bool ParseUrl(const std::string& url, ParsedUrl& out)
    {
        std::wstring wideUrl(url.begin(), url.end());

        URL_COMPONENTS components;
        ZeroMemory(&components, sizeof(components));
        components.dwStructSize = sizeof(components);

        wchar_t hostBuffer[256];
        wchar_t pathBuffer[2048];
        ZeroMemory(hostBuffer, sizeof(hostBuffer));
        ZeroMemory(pathBuffer, sizeof(pathBuffer));

        components.lpszHostName = hostBuffer;
        components.dwHostNameLength = 256;
        components.lpszUrlPath = pathBuffer;
        components.dwUrlPathLength = 2048;

        if (!WinHttpCrackUrl(wideUrl.c_str(), static_cast<DWORD>(wideUrl.size()), 0, &components))
        {
            return false;
        }

        out.host = hostBuffer;
        out.path = pathBuffer;
        out.port = components.nPort;
        out.secure = (components.nScheme == INTERNET_SCHEME_HTTPS);
        return true;
    }
}

bool Http::Get(const std::string& url, const std::vector<std::pair<std::string, std::string>>& headers, std::string& response)
{
    ParsedUrl parsedUrl;
    if (!ParseUrl(url, parsedUrl))
    {
        return false;
    }

    HINTERNET session = WinHttpOpen(L"OrbitScope/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session)
    {
        return false;
    }

    HINTERNET connect = WinHttpConnect(session, parsedUrl.host.c_str(), parsedUrl.port, 0);
    if (!connect)
    {
        WinHttpCloseHandle(session);
        return false;
    }

    DWORD flags = parsedUrl.secure ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request = WinHttpOpenRequest(connect, L"GET", parsedUrl.path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request)
    {
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return false;
    }

    std::wstring headerString;
    for (const auto& header : headers)
    {
        std::wstring key(header.first.begin(), header.first.end());
        std::wstring value(header.second.begin(), header.second.end());
        headerString += key + L": " + value + L"\r\n";
    }

    bool success = false;

    if (WinHttpSendRequest(request, headerString.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headerString.c_str(), headerString.empty() ? 0 : static_cast<DWORD>(headerString.size()), WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
        && WinHttpReceiveResponse(request, NULL))
    {
        DWORD available = 0;
        response.clear();

        while (WinHttpQueryDataAvailable(request, &available) && available > 0)
        {
            std::vector<char> buffer(available);
            DWORD bytesRead = 0;

            if (WinHttpReadData(request, buffer.data(), available, &bytesRead))
            {
                response.append(buffer.data(), bytesRead);
            }
            else
            {
                break;
            }
        }

        success = true;
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);

    return success;
}