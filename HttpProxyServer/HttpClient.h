/*
 * HttpClient.h
 *
 *  Created on: 2014-12-24
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#ifndef HttpClient_H_
#define HttpClient_H_

#include <curl/curl.h>

#include <string>
#include <list>
using namespace std;

class HttpClient;
class IHttpClientCallback {
public:
	virtual ~IHttpClientCallback(){};
	virtual void onReceiveBody(HttpClient* client, const char* buf, int size) = 0;
	virtual int onSendBody(HttpClient* client, char* buf, int size) = 0;
};

class HttpClient {
public:
	HttpClient();
	virtual ~HttpClient();

	static void Init();
	bool Request(const string& url, const list<string> headers);
	void Stop();

	void SetCallback(IHttpClientCallback *callback);

private:
	static size_t CurlWriteHandle(void *buffer, size_t size, size_t nmemb, void *data);
	void HttpWriteHandle(void *buffer, size_t size, size_t nmemb);

	static size_t CurlReadHandle(void *buffer, size_t size, size_t nmemb, void *data);
	int HttpReadHandle(void *buffer, size_t size, size_t nmemb);

	static size_t CurlProgress(
			void *data,
            double downloadTotal,
            double downloadNow,
            double uploadTotal,
            double uploadNow
            );
	size_t HttpProgress(
            double downloadTotal,
            double downloadNow,
            double uploadTotal,
            double uploadNow
            );

	IHttpClientCallback *mpIHttpClientCallback;

	CURL *mpCURL;
	string mUrl;
	string mContentType;
	double mContentLength;

	// stop manually
	bool mbStop;

	// application timeout
	double mdUploadTotal;
	double mdUploadLast;
	double mdUploadLastTime;
	double mdDownloadTotal;
	double mdDownloadLast;
	double mdDownloadLastTime;
};

#endif /* HttpClient_H_ */
