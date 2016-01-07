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
#include <map>
using namespace std;

typedef map<string, string> Parameters;

typedef enum HttpType {
	GET,
	POST,
	UNKNOW,
};

class DataHttpParser : public DataParser {
public:
	DataHttpParser();
	virtual ~DataHttpParser();

	int ParseData(char* buffer, int len);

	const string& GetParam(const char* key);
	const string& GetPath();
	HttpType GetType();

	void SetSendMaxSeq(int seq);
	bool IsFinishSeq(int seq);

	void Reset();

private:
	HttpType mHttpType;
	int miContentLength;
	Parameters mParameters;
	string mPath;

	int miSendMaxSeq;
	KMutex mSeqMutex;

	bool ParseFirstLine(char* buffer);
	void ParseParameters(char* buffer);
};

#endif /* DATAHTTPPARSER_H_ */
