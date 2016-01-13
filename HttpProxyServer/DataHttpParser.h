/*
 * DataHttpParser.h
 *
 *  Created on: 2015-9-28
 *      Author: Max
 */

#ifndef DATAHTTPPARSER_H_
#define DATAHTTPPARSER_H_

#include "DataParser.h"
#include "MessageList.h"

#include <common/Arithmetic.hpp>
#include <common/KMutex.h>

#include <stdio.h>
#include <stdlib.h>

#include <ctype.h>
#include <algorithm>

#include <string>
#include <list>
using namespace std;

typedef list<string> Headers;

typedef enum HttpType {
	GET,
	POST,
	UNKNOW,
};

class DataHttpParser : public DataParser {
public:
	DataHttpParser();
	virtual ~DataHttpParser();

	int ParseData(const char* buffer, int len);

	void Reset();

	string GetUrl();
	const char* GetBody();
	int GetContentLength();
	const Headers& GetHeaders();

private:
	bool ParseFirstLine(char* buffer);

	string mHost;
	string mPath;
	int miContentLength;

	char mBuffer[MAX_BUFFER_LEN];
	bool mbReceiveHeaderFinish;
	int mIndex;
	int mHeaderIndex;

	HttpType mHttpType;

	Headers mHeaders;
};

#endif /* DATAHTTPPARSER_H_ */
