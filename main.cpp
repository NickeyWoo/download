#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <curl/curl.h>

#include <utility>
#include <string>
#include <map>

#include <boost/format.hpp>
#include <boost/regex.hpp>

#define RET_CURL(fun, ...)																\
	if((retCode = fun(__VA_ARGS__)) != CURLE_OK)										\
	{																					\
		printf("error[%s:%d]: %s\n", __FILE__, __LINE__, curl_easy_strerror(retCode));	\
		return -1;																		\
	}

CURL* pstCURLHandle = NULL;
bool bOutHeader = false;
bool bOutHttpStatus = false;
bool bNoOutContent = false;
const char* szUserAgent = NULL;
const char* szUrl = NULL;

std::string strHeader;
std::string strContent;
int iHttpStatus = 0;

size_t HttpWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
	char* pbuffer = (char*)malloc(size * nmemb + 1);
	memcpy(pbuffer, ptr, size * nmemb);
	pbuffer[size * nmemb] = 0;
	strContent.append(pbuffer);
	free(pbuffer);
	return size * nmemb;
}

size_t HttpHeaderCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
	char* pbuffer = (char*)malloc(size * nmemb + 1);
	memcpy(pbuffer, ptr, size * nmemb);
	pbuffer[size * nmemb] = 0;
	strHeader.append(pbuffer);
	free(pbuffer);
	return size * nmemb;
}

void print_usage()
{
	printf("usage: download [-help] [-status] [-nocontent] [-head] [-useragent \"user agent\"] [url]\n");
}

int main(int argc, char* argv[])
{
	for(int i=1; i<argc; ++i)
	{
		if(strcmp(argv[i], "-help") == 0)
		{
			print_usage();
			return -1;
		}
		else if(strcmp(argv[i], "-nocontent") == 0)
		{
			bNoOutContent = true;
		}
		else if(strcmp(argv[i], "-status") == 0)
		{
			bOutHttpStatus = true;
		}
		else if(strcmp(argv[i], "-head") == 0)
		{
			bOutHeader = true;
		}
		else if(strcmp(argv[i], "-useragent") == 0)
		{
			++i;
			if(i < argc)
				szUserAgent = argv[i];
			else
			{
				print_usage();
				return -1;
			}
		}
		else
		{
			szUrl = argv[i];
		}
	}

	CURLcode retCode = CURLE_OK;
	RET_CURL(curl_global_init, CURL_GLOBAL_ALL);
	pstCURLHandle = curl_easy_init();
	if(pstCURLHandle == NULL)
	{
		printf("error[%s:%d]: create CURL handle fail.\n", __FILE__, __LINE__);
		return -1;
	}

	RET_CURL(curl_easy_setopt, pstCURLHandle, CURLOPT_HEADER, 0);
	RET_CURL(curl_easy_setopt, pstCURLHandle, CURLOPT_WRITEFUNCTION, HttpWriteCallback);
	RET_CURL(curl_easy_setopt, pstCURLHandle, CURLOPT_HEADERFUNCTION, HttpHeaderCallback);

	if(szUserAgent != NULL)
		RET_CURL(curl_easy_setopt, pstCURLHandle, CURLOPT_USERAGENT, szUserAgent);

	if(szUrl == NULL)
	{
		char buffer[4097];
		int size = 0;
		int len = 4096;
		int offset = 0;
		memset(buffer, 0, 4097);
		while(-1 != (size = read(STDIN_FILENO, buffer + offset, len)))
		{
			if(size == 0)
				break;

			char* pStart = buffer;
			char* pEnd = NULL;
			while((pEnd = strchr(pStart, '\n')) != NULL)
			{
				pEnd[0] = 0x0;

				RET_CURL(curl_easy_setopt, pstCURLHandle, CURLOPT_URL, pStart);
				RET_CURL(curl_easy_perform, pstCURLHandle);

				pStart = pEnd + 1;
			}

			strcpy(buffer, pStart);
			offset = strlen(pStart);
			len = 4096 - offset;
			memset(buffer + offset, 0, len);
		}
	}
	else
	{
		RET_CURL(curl_easy_setopt, pstCURLHandle, CURLOPT_URL, szUrl);
		RET_CURL(curl_easy_perform, pstCURLHandle);
		if(bOutHttpStatus)
		{
			boost::regex rgxHttpStatus("^HTTP/([^ ]+) ([0-9]+)");
			boost::smatch what;
			if(boost::regex_search(strHeader, what, rgxHttpStatus))
				printf("%d\n", atoi(what[2].str().c_str()));
			else
				printf("500\n");
		}
		if(bOutHeader)
			printf("%s", strHeader.c_str());
		if(!bNoOutContent)
			printf("%s", strContent.c_str());
	}

	curl_easy_cleanup(pstCURLHandle);
	curl_global_cleanup();
	return 0;
}

