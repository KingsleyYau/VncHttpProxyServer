/*
 * KTcpSocket.cpp
 *
 *  Created on: 2014-12-24
 *      Author: Max.Chiu
 *      Email: Kingsleyyau@gmail.com
 */

#include "KTcpSocket.h"
#include "config.h"

KTcpSocket::KTcpSocket() {
	// TODO Auto-generated constructor stub
	mIp = "";
	miPort = 0;
	mbConnected = false;
}

KTcpSocket::~KTcpSocket() {
	// TODO Auto-generated destructor stub
}

KTcpSocket KTcpSocket::operator=(const KTcpSocket &obj) {
	this->mSocket = obj.mSocket;
	this->mIp = obj.mIp;
	this->miPort = obj.miPort;
	this->mbConnected = obj.mbConnected;
	this->mbBlocking = obj.mbBlocking;
	return *this;
}

void KTcpSocket::SetConnnected() {
	mbConnected = true;
}

bool KTcpSocket::IsConnected() {
	return mbConnected;
}

void KTcpSocket::SetAddress(sockaddr_in addr) {
	mIp = KSocket::IpToString(addr.sin_addr.s_addr);
	miPort = ntohs(addr.sin_port);
}

string KTcpSocket::GetIP() {
	return mIp;
}

int KTcpSocket::GetPort() {
	return miPort;
}

int KTcpSocket::Connect(string strAddress, unsigned int uiPort, bool bBlocking) {
	int iRet = -1, iFlag = 1;
	sockaddr_in dest;
	hostent* hent = NULL;

	mIp = strAddress;
	miPort = uiPort;

	if ( (mSocket = socket(AF_INET, SOCK_STREAM, 0)) >= 0 ) {

		bool bCanSelect = (mSocket > 1023)?false:true;

		if( !bCanSelect && !bBlocking ) {
			printf("KTcpSocket::Connect( nonblocking and can not be select : %s ) \n", bCanSelect?"true":"false");
			iRet = -1;
			goto EXIT_ERROR_TCP;
		}

		bzero(&dest, sizeof(dest));
		dest.sin_family = AF_INET;
		dest.sin_port = htons(uiPort);

		dest.sin_addr.s_addr = inet_addr((const char*)strAddress.c_str());
		if (dest.sin_addr.s_addr == INADDR_NONE) {
			if ((hent = gethostbyname((const char*)strAddress.c_str())) != NULL) {
				dest.sin_family = hent->h_addrtype;
				memcpy((char*)&dest.sin_addr, hent->h_addr, hent->h_length);
				mIp = KSocket::IpToString(dest.sin_addr.s_addr);
				printf("KTcpSocket::Connect( gethostbyname address : %s, ip : %s ) \n",
						(const char*)strAddress.c_str(), mIp.c_str());

			} else {
				printf("KTcpSocket::Connect( gethostbyname address : %s fail, error : %s ) \n",
						(const char*)strAddress.c_str(), hstrerror(h_errno));
				iRet = -1;
				goto EXIT_ERROR_TCP;
			}
		}

		setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char *)&iFlag, sizeof(iFlag));

		iFlag = 1500;
		setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (const char *)&iFlag, sizeof(iFlag));

		iFlag = 1;
		setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&iFlag, sizeof(iFlag));

		if(setBlocking(!bCanSelect)) {

		}
		else {
			iRet = -1;
			goto EXIT_ERROR_TCP;
		}

		if ( connect(mSocket, (struct sockaddr *)&dest, sizeof(dest)) != -1) {
			iRet = 1;
		}
		else {
			fd_set wset;
			struct timeval tout;
			tout.tv_sec = 10;
			tout.tv_usec = 0;
			FD_ZERO(&wset);
			FD_SET(mSocket, &wset);
			int iRetS = select(mSocket + 1, NULL, &wset, NULL, &tout);
			if (iRetS > 0) {
//				int error, len;
//				getsockopt(mSocket, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
//				if(error == 0) {
//					iRet = 1;
//
//				} else {
//					iRet = -1;
//					printf("KTcpSocket::Connect( connect timeout ) \n");
//					goto EXIT_ERROR_TCP;
//				}
				iRet = 1;

			} else {
				printf("KTcpSocket::Connect( connect select timeout ) \n");
				iRet = -1;
				goto EXIT_ERROR_TCP;
			}
		}

		if(setBlocking(bBlocking)) {

		}
		else {
			printf("KTcpSocket::Connect( set blocking fail ) \n");
			iRet = -1;
			goto EXIT_ERROR_TCP;
		}

	    /*deal with the tcp keepalive
	      iKeepAlive = 1 (check keepalive)
	      iKeepIdle = 600 (active keepalive after socket has idled for 10 minutes)
	      KeepInt = 60 (send keepalive every 1 minute after keepalive was actived)
	      iKeepCount = 3 (send keepalive 3 times before disconnect from peer)
	     */
	    int iKeepAlive = 1, iKeepIdle = 15, KeepInt = 15, iKeepCount = 2;
	    if (setsockopt(mSocket, SOL_SOCKET, SO_KEEPALIVE, (void*)&iKeepAlive, sizeof(iKeepAlive)) < 0) {
	    	iRet = -1;
	    	goto EXIT_ERROR_TCP;
	    }
	    if (setsockopt(mSocket, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&iKeepIdle, sizeof(iKeepIdle)) < 0) {
	    	iRet = -1;
	    	goto EXIT_ERROR_TCP;
	    }
	    if (setsockopt(mSocket, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&KeepInt, sizeof(KeepInt)) < 0) {
	    	iRet = -1;
	    	goto EXIT_ERROR_TCP;
	    }
	    if (setsockopt(mSocket, IPPROTO_TCP, TCP_KEEPCNT, (void *)&iKeepCount, sizeof(iKeepCount)) < 0) {
	    	iRet = -1;
	    	goto EXIT_ERROR_TCP;
	    }

	    iRet = 1;
	} else {
		printf("KTcpSocket::Connect( create socket fail ) \n");
	}

EXIT_ERROR_TCP:

	if ( iRet != 1 ) {
		Close();
		printf("KTcpSocket::Connect( connect fail ) \n");

	} else {
		mbConnected = true;
	}
	return iRet;
}
int KTcpSocket::SendData(char* pBuffer, unsigned int uiSendLen, unsigned int uiTimeout) {
	unsigned int uiBegin = GetTick();
	string log = "";
	int iOrgLen = uiSendLen;
	int iRet = -1, iRetS = -1;
	int iSendedLen = 0;
	fd_set wset;

    if(!IsConnected()) {
        goto EXIT_TCP_SEND;
    }

	if(mbBlocking) {
		iRet = send(mSocket, pBuffer, uiSendLen, 0);
		goto EXIT_TCP_SEND;
	}
	else {
		while (true) {
			struct timeval tout;
			tout.tv_sec = uiTimeout / 1000;
			tout.tv_usec = (uiTimeout % 1000) * 1000;
			FD_ZERO(&wset);
			FD_SET(mSocket, &wset);
			iRetS = select(mSocket + 1, NULL, &wset, NULL, &tout);
			if (iRetS == -1) {
				iRet = -1;
				goto EXIT_TCP_SEND;
			} else if (iRetS == 0) {
				iRet = iSendedLen;
				goto EXIT_TCP_SEND;
			}

			iRet = send(mSocket, pBuffer, uiSendLen, 0);
			if (iRet == -1 || (iRetS == 1 && iRet == 0)) {
				usleep(100);
				if (EWOULDBLOCK == errno) {
					if (IsTimeout(uiBegin, uiTimeout)) {
						iRet = iSendedLen;
						goto EXIT_TCP_SEND;
					}
					continue;
				} else {
					if (iSendedLen == 0){
						iRet = -1;
						goto EXIT_TCP_SEND;
					}
					iRet = iSendedLen;
					goto EXIT_TCP_SEND;
				}
			} else if (iRet == 0) {
				iRet = iSendedLen;
				goto EXIT_TCP_SEND;
			}
			pBuffer += iRet;
			iSendedLen += iRet;
			uiSendLen -= iRet;
			if (iSendedLen >= iOrgLen) {
				iRet = iSendedLen;
				goto EXIT_TCP_SEND;
			}
			if (IsTimeout(uiBegin, uiTimeout)) {
				iRet = iSendedLen;
				goto EXIT_TCP_SEND;
			}
		}
	}

EXIT_TCP_SEND:
	if(iRet == -1) {
		Close();
	}
	else {
	}
	return iRet;
}

int KTcpSocket::RecvData(char* pBuffer, unsigned int uiRecvLen, bool bRecvAll, bool* pbAlive, unsigned int uiTimeout) {
	unsigned int uiBegin = GetTick();
	int iOrgLen = uiRecvLen;
	int iRet = -1, iRetS = -1;
	int iRecvedLen = 0;
	bool bAlive = true;
	fd_set rset;

    if(!IsConnected()) {
    	bAlive = false;
        goto EXIT_TCP_RECV;
    }

	if(mbBlocking) {
		iRet = recv(mSocket, pBuffer, uiRecvLen, 0);
		if(iRet > 0) {
		}
		else {
			bAlive = false;
		}
		goto EXIT_TCP_RECV;
	}
	else {
		while (true) {
			timeval tout;
			tout.tv_sec = uiTimeout / 1000;
			tout.tv_usec = (uiTimeout % 1000) * 1000;
			FD_ZERO(&rset);
			FD_SET(mSocket, &rset);
			iRetS = select(mSocket + 1, &rset, NULL, NULL, &tout);

			if (iRetS == -1) {
				iRet = -1;
				bAlive = false;
				goto EXIT_TCP_RECV;
			} else if (iRetS == 0) {
				iRet = iRecvedLen;
				goto EXIT_TCP_RECV;
			}
            else {
                iRet = recv(mSocket, pBuffer, uiRecvLen, 0);
                if(iRet == 0) {
                    iRet = iRecvedLen;
                    bAlive = false;
                    goto EXIT_TCP_RECV;
                }
                else if (iRet == -1){
                    usleep(1000);
                    if (EWOULDBLOCK == errno || EINTR == errno){
                        if (IsTimeout(uiBegin, uiTimeout)){
                            iRet = iRecvedLen;
                            goto EXIT_TCP_RECV;
                        }
                        continue;
                    } else {
                        iRet = iRecvedLen;
                        bAlive = false;
                        goto EXIT_TCP_RECV;
                    }
                }

                pBuffer += iRet;
                iRecvedLen += iRet;
                uiRecvLen -= iRet;
                if (iRecvedLen >= iOrgLen) {
                    iRet = iRecvedLen;
                    goto EXIT_TCP_RECV;
                }
                if (!bRecvAll) {
                    iRet = iRecvedLen;
                    goto EXIT_TCP_RECV;
                }
                if (IsTimeout(uiBegin, uiTimeout)){
                    iRet = iRecvedLen;
                    goto EXIT_TCP_RECV;
                }
            }
		}
	}

EXIT_TCP_RECV:
	if(mbBlocking && bAlive == false) {
		Close();
	}
	else if(iRet == -1 || bAlive == false) {
		Close();
	}

    if(pbAlive != NULL) {
        *pbAlive = bAlive;
    }
	return iRet;
}

/*
 * Server
 */
bool KTcpSocket::Bind(unsigned int iPort, string ip) {
	bool bFlag = false;
	if((mSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		// create socket error;
		bFlag = false;
		goto EXIT_TCP_BIND;
	}
	else {
	}

	struct sockaddr_in localAddr;
	bzero(&localAddr, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	if(ip.length() > 0) {
		inet_aton(ip.c_str(), &localAddr.sin_addr);
	}
	else {
		localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}

	localAddr.sin_port = htons(iPort);
	if(bind(mSocket, (struct sockaddr *)&localAddr, sizeof(localAddr)) < 0) {
		// bind socket error
		bFlag = false;
		goto EXIT_TCP_BIND;
	}
	else {
		bFlag = true;
	}

EXIT_TCP_BIND:
	if(!bFlag) {
		Close();
	}
    return bFlag;
}

bool KTcpSocket::Listen(int maxSocketCount, bool bBlocking) {
	bool bFlag = false;
    if (listen(mSocket, maxSocketCount) == -1) {
		bFlag = false;
		goto EXIT_TCP_LISTEN;
    }
    else {
		bFlag = true;
	}

    mbBlocking = bBlocking;
EXIT_TCP_LISTEN:
	if(!bFlag) {
		Close();
	}
	return bFlag;
}
KTcpSocket KTcpSocket::Accept(unsigned int uiTimeout, bool bBlocking) {
    KTcpSocket clientSocket;
    socket_type accpetSocket = -1;

    struct sockaddr_in remoteAddr;
	bzero(&remoteAddr, sizeof(remoteAddr));
	socklen_t iRemoteAddrLen = sizeof(struct sockaddr_in);

	int iRet = -1;
	if(mbBlocking) {
	}
	else {
		struct timeval tout;
		tout.tv_sec = uiTimeout / 1000;
		tout.tv_usec = (uiTimeout % 1000) * 1000;
		fd_set rset;
		FD_ZERO(&rset);
		FD_SET(mSocket, &rset);
		int iRetS = select(mSocket + 1, &rset, NULL, NULL, &tout);
		if (iRetS == -1) {
			iRet = -1;
			goto EXIT_TCP_ACCEPT;
		} else if (iRetS == 0) {
			iRet = 0;
			goto EXIT_TCP_ACCEPT;
		}
	}

	accpetSocket = accept(mSocket, (struct sockaddr *)&remoteAddr, &iRemoteAddrLen);

	if(accpetSocket != -1) {

	    /*deal with the tcp keepalive
	      iKeepAlive = 1 (check keepalive)
	      iKeepIdle = 600 (active keepalive after socket has idled for 10 minutes)
	      KeepInt = 60 (send keepalive every 1 minute after keepalive was actived)
	      iKeepCount = 3 (send keepalive 3 times before disconnect from peer)
	     */
	    int iKeepAlive = 1, iKeepIdle = 15, KeepInt = 15, iKeepCount = 2;
	    if (setsockopt(accpetSocket, SOL_SOCKET, SO_KEEPALIVE, (void*)&iKeepAlive, sizeof(iKeepAlive)) < 0) {
	    	iRet = -1;
	    	goto EXIT_TCP_ACCEPT;
	    }
	    if (setsockopt(accpetSocket, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&iKeepIdle, sizeof(iKeepIdle)) < 0) {
	    	iRet = -1;
	    	goto EXIT_TCP_ACCEPT;
	    }
	    if (setsockopt(accpetSocket, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&KeepInt, sizeof(KeepInt)) < 0) {
	    	iRet = -1;
	    	goto EXIT_TCP_ACCEPT;
	    }
	    if (setsockopt(accpetSocket, IPPROTO_TCP, TCP_KEEPCNT, (void *)&iKeepCount, sizeof(iKeepCount)) < 0) {
	    	iRet = -1;
	    	goto EXIT_TCP_ACCEPT;
	    }

		clientSocket.setScoket(accpetSocket);
		clientSocket.SetAddress(remoteAddr);
		clientSocket.SetConnnected();
		clientSocket.setBlocking(bBlocking);

		iRet = 1;
	}

EXIT_TCP_ACCEPT:
	if(iRet == -1) {
		Close();
	}
	return clientSocket;
}

void KTcpSocket::Close() {
	KSocket::Close();
	mbConnected = false;
}
