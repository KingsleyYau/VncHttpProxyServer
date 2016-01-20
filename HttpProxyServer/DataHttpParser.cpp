/*
 * DataHttpParser.cpp
 *
 *  Created on: 2015-9-28
 *      Author: Max
 */

#include "DataHttpParser.h"

DataHttpParser::DataHttpParser() {
	// TODO Auto-generated constructor stub
	Reset();
}

DataHttpParser::~DataHttpParser() {
	// TODO Auto-generated destructor stub
}

void DataHttpParser::Reset() {
	mbReceiveHeaderFinish = false;
	miContentLength = -1;
	mIndex = 0;
	mHeaderIndex = 0;
	memset(mBuffer, '\0', sizeof(mBuffer));
}

int DataHttpParser::ParseData(const char* buffer, int len) {
	int ret = 0;

	int recvLen = (len < MAX_BUFFER_LEN - mIndex)?len:MAX_BUFFER_LEN - mIndex;
	if( recvLen > 0 ) {
		memcpy(mBuffer + mIndex, buffer, recvLen);
		mIndex += recvLen;
		mBuffer[mIndex + 1] = '\0';

	} else {
		return -1;
	}

//	printf("# DataHttpParser::ParseData( mbReceiveHeaderFinish : %s ) \n", mbReceiveHeaderFinish?"true":"false");

	if( !mbReceiveHeaderFinish ) {
		string headers = mBuffer;
		int headerIndex = 0;
		string::size_type pos = headers.find("\r\n\r\n");

		if( pos != string::npos ) {
			headerIndex = pos + strlen("\r\n\r\n");

			mbReceiveHeaderFinish = true;

			// Read first line
			pos = headers.find("\r\n");
			if( pos != string::npos ) {
				string firstLine = headers.substr(0, pos);
				char firstLineBuff[4096];
				strcpy(firstLineBuff, firstLine.c_str());

//				LogManager::GetLogManager()->Log(
//						LOG_MSG,
//						"DataHttpParser::ParseData( "
//						"firstLine : %s, "
//						"headers : %s "
//						")",
//						firstLine.c_str(),
//						headers.c_str()
//						);
//				headers = headers.substr(pos + strlen("\r\n"), headers.length() - (pos + strlen("\r\n")));
				if( ParseFirstLine(firstLineBuff) ) {
					string header;
					string::size_type posStart, posEnd;
					string::size_type posPre = pos + strlen("\r\n");

					// Get all headers
					pos = headers.find("\r\n", posPre);
					while( pos != string::npos ) {
						header = headers.substr(posPre, pos - posPre);
//						printf("# DataHttpParser::ParseData( header : %s ) \n", header.c_str());
//						LogManager::GetLogManager()->Log(
//								LOG_MSG,
//								"DataHttpParser::ParseData( "
//								"header : %s "
//								")",
//								header.c_str()
//								);

						if( header.length() > 0 ) {
							// Get Host
							posStart = header.find("Host: ");
							if( posStart != string::npos ) {
								mHost = header.substr(posStart + strlen("Host: "), header.length() - (posStart + strlen("Host:")));
//								printf("# DataHttpParser::ParseData( Host: %s ) \n", mHost.c_str());
							}

							// Get Content-Length
							posStart = header.find("Content-Length: ");
							if( posStart != string::npos ) {
								string contentLength = header.substr(posStart + strlen("Content-Length: "), header.length() - (posStart + strlen("Content-Length:")));
//								printf("# DataHttpParser::ParseData( Content-Length: %s ) \n", contentLength.c_str());
								miContentLength = atoi(contentLength.c_str());
							}

							mHeaders.push_back(header);
						}

						posPre = pos + strlen("\r\n");
						pos = headers.find("\r\n", posPre);
					}
				}
			}

			if( mIndex > headerIndex ) {
				memcpy(mBuffer, mBuffer + headerIndex, mIndex - headerIndex);

			} else {
				mBuffer[0] = '\0';
				mIndex = 0;

			}
		}
	}

	// Receive all body
	if( mbReceiveHeaderFinish ) {
		if( miContentLength == -1 || (mIndex == miContentLength) ) {
			ret = 1;
		}
	}

	return ret;
}

string DataHttpParser::GetUrl() {
	string url = "";

	if( mHost.length() != 0 ) {
		if( string::npos == mPath.find(mHost) ) {
			url += mHost;
			url += mPath;

		} else {
			url += mPath;

		}

	} else {
		url += mPath;
	}

	if( string::npos == url.find("http://") ) {
		url = "http://" + url;
	}

	return url;
}

const char* DataHttpParser::GetBody() {
	return mBuffer;
}

int DataHttpParser::GetContentLength() {
	return miContentLength;
}

const Headers& DataHttpParser::GetHeaders() {
	return mHeaders;
}

bool DataHttpParser::ParseFirstLine(char* buffer) {
	bool bFlag = true;
	char* p = NULL;
	int j = 0;

	char *pFirst = NULL;

	p = strtok_r(buffer, " ", &pFirst);
	while( p != NULL ) {
		switch(j) {
		case 0:{
			// type
			if( strcmp("GET", p) == 0 ) {
				mHttpType = GET;
			} else if( strcmp("POST", p) == 0 ) {
				mHttpType = POST;
			} else {
				bFlag = false;
				break;
			}
		}break;
		case 1:{
			// path and parameters
			mPath = p;
			string::size_type pos = mPath.find("http://");
			if( string::npos != pos ) {
				mPath = mPath.substr(pos + strlen("http://"), mPath.length() - (pos + strlen("http://")));
			}
//			printf("# DataHttpParser::ParseFirstLine( mPath : %s ) \n", mPath.c_str());

		}break;
		default:break;
		};

		j++;
		p = strtok_r(NULL, " ", &pFirst);
	}

	return bFlag;
}
