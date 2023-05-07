#pragma once
#include <windows.h>
#include <wininet.h>
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <cstdlib>

// Include the "Wininet" library into our project (there are other ways to do this). 
#pragma comment (lib, "Wininet.lib")
//////////////////////////////////////
// https://gist.github.com/AhnMo/5cf37bbd9a6fa2567f99ac0528eaa185?permalink_comment_id=4063365
// https://docs.microsoft.com/en-us/windows/win32/wininet/using-wininet
class HttpClient
{
	HINTERNET hSession {};
	HINTERNET hConnect {};

	std::string authScheme;
	std::string authToken;

	// The name of the app requesting the data.
	const std::string userAgent = "itcs2550/app";

	// The header indicating the format of the returned data.
	const std::string acceptType = "application/json";

	// Header container
	std::map<std::string, std::string> respHeaders;

public:

	// This function will attempt to locate the header 
	// value given a key.
	std::string GetHeaderValue(const std::string key)
	{
		auto search = respHeaders.find(key);
		if (search != respHeaders.end())
		{
			return search->second;
		}

		return "";
	}

protected:

	// Override this to get the data from the GET method call.
	virtual void Data(const char* data, const unsigned int size) {}

	virtual void StartOfData() {}
	virtual void EndOfData() {}

	int nPort = INTERNET_DEFAULT_HTTPS_PORT;

public:

	// Constructor...
	HttpClient()
	{
		// The header indicating the format of the returned data
		const std::string acceptType = "application/json";

		hSession = InternetOpenA(userAgent.c_str(), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		if (hSession == NULL)
			std::cerr << "InternetOpen error : " << GetLastError() << std::endl;
	}

	// Destructor...
	~HttpClient()
	{
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hSession);
	}

	///
	void SetAuthToken(const std::string scheme, const std::string token = "") 
	{ 
		authScheme = scheme;
		authToken = token; 
	}

	///
	const std::string AddQueryParameters(const std::string uri, const std::map<std::string, std::string>& qp = {})
	{
		if (!qp.size())
			return uri;

		/// Do the query parameters.
		std::string path(uri + '?');

		for (auto& p : qp)
		{
			path += p.first + "=" + p.second + '&';
		}

		path.pop_back();
		return path;
	}

	/// Connect to the server...
	bool Connect(const std::string server_name, const int p = INTERNET_DEFAULT_HTTPS_PORT)
	{
		nPort = p;

		if (hSession)
		{
			hConnect = InternetConnectA(hSession, server_name.c_str(), nPort, "", "", INTERNET_SERVICE_HTTP, 0, 0);
			if (hConnect == NULL)
				std::cerr << "InternetConnect error : " << GetLastError() << std::endl;
		}

		return hConnect != NULL;
	}

	/// Invoke an HTTP GET method...
	/// If there are query parameters then add them so that they look something like: "http://example.com/path?firstname=joe&lastname=blow"
	bool Get(const std::string uri, const std::map<std::string, std::string> qp = {})
	{
		/// Add the query parameters.
		std::string u = AddQueryParameters(uri, qp);

		///
		DWORD dwFlags = (nPort == INTERNET_DEFAULT_HTTPS_PORT) ? INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_SECURE : INTERNET_FLAG_DONT_CACHE;
		PCSTR rgpszAcceptTypes[] = { acceptType.c_str(), NULL };
		HINTERNET hRequest = HttpOpenRequestA(hConnect, "GET", u.c_str(), NULL, NULL, rgpszAcceptTypes, dwFlags, 0);
		if (hRequest == NULL)
		{
			std::cerr << "HttpOpenRequest error : " << GetLastError() << std::endl;
			return false;
		}

		///
		std::string connHeader("Connection: keep-alive");
		HttpAddRequestHeadersA(hRequest, connHeader.c_str(), static_cast<DWORD>(connHeader.length()), 0);

		if (!authScheme.empty() && !authToken.empty())
		{
			std::string authHeader("Authorization: " + authScheme + " " + authToken + "\r\n");
			HttpAddRequestHeadersA(hRequest, authHeader.c_str(), static_cast<DWORD>(authHeader.length()), 0);
		}

		///
		while (!HttpSendRequest(hRequest, NULL, 0, 0, 0))
		{
			std::cerr << "HttpOpenRequest error : " << GetLastError() << std::endl;
			return false;
		}
		///
		SampleCodeOne(hRequest);

		///
		const int BUFFER_SIZE = 4096;
		char buffer[BUFFER_SIZE] {};

		// Inform the derived classes
		StartOfData();

		while (true)
		{
			DWORD dwBytesRead;
			BOOL bRead;

			bRead = InternetReadFile(hRequest, buffer, BUFFER_SIZE, &dwBytesRead);
			if (!bRead)
			{
				std::cerr << "InternetReadFile error : " << GetLastError() << std::endl;
			}
			else
			{
				if (dwBytesRead == 0)
					break;

				// Inform the derived classes
				Data(buffer, dwBytesRead);
			}
		}

		// Inform the derived classes
		EndOfData();

		InternetCloseHandle(hRequest);
		return true;
	}
	
	// Retrieving Headers Using a Constant
	// https://docs.microsoft.com/en-us/windows/win32/wininet/retrieving-http-headers
	bool SampleCodeOne(HINTERNET hHttp)
	{
		char* lpOutBuffer {};
		DWORD dwSize = 0;

	retry:

		// This call will fail on the first pass, because
		// no buffer is allocated.
		if (!HttpQueryInfoA(hHttp, HTTP_QUERY_RAW_HEADERS_CRLF,(LPVOID)lpOutBuffer, &dwSize, NULL))
		{
			if (GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND)
			{
				// Code to handle the case where the header isn't available.
				return true;
			}
			else
			{
				// Check for an insufficient buffer.
				if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
				{
					// Allocate the necessary buffer.
					lpOutBuffer = new char[dwSize];

					// Retry the call.
					goto retry;
				}
				else
				{
					// Error handling code.
					if (lpOutBuffer)
					{
						delete[] lpOutBuffer;
					}
					return false;
				}
			}
		}

		if (lpOutBuffer)
		{
			ParseHeaders(lpOutBuffer);
			delete[] lpOutBuffer;
		}

		return true;
	}
	///
	void ParseHeaders(const char* pData)
	{
		std::string header;
		std::istringstream resp(pData);
		while (std::getline(resp, header) && header != "\r")
		{
			size_t index = header.find(':', 0);
			if (index != std::string::npos)
			{
				respHeaders.insert(std::make_pair(trim(header.substr(0, index)), trim(header.substr(index + 1))));
			}
		}
	}

	// trim from start (in place)
	void ltrim(std::string& s) const  {	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [] (int ch) { return !std::isspace(ch); })); }

	// trim from end (in place)
	void rtrim(std::string& s) const { s.erase(std::find_if(s.rbegin(), s.rend(), [] (int ch) { return !std::isspace(ch); }).base(), s.end()); 	}

	// trim from both ends (in place)
	std::string trim(std::string s) const 
	{
		ltrim(s);
		rtrim(s);
		return s;
	}
};
