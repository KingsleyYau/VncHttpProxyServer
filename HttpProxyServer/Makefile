# Copyright (C) 2015 The VncHttpProxyServer
# HttpProxyServer Makefile
#
# Created on: 2015/10/10
# Author: Max.Chiu
# Email: Kingsleyyau@gmail.com
#

export MAKE	:=	make

release=0
ifeq ($(release), 1)
CXXFLAGS = -O3 
else
CXXFLAGS = -O2 -g
endif

CXXFLAGS +=	-Wall -fmessage-length=0 -Wunused-variable
CXXFLAGS +=	-I. -Iinclude -Icurl/include

LIBS =		-L. \
			-Wl,-Bdynamic -Lcurl/lib/.libs -lcurl \
			-Wl,-Bdynamic -ldl -lz -lpthread 

COMMONOBJ =	common/LogFile.o common/md5.o common/KThread.o \
			common/ConfFile.o common/Arithmetic.o common/KSocket.o common/KTcpSocket.o
OBJS =		server.o LogManager.o MessageList.o DataParser.o DataHttpParser.o \
			HttpProxyServer.o TcpProxyClient.o HttpClient.o HttpRequest.o
OBJS +=		$(COMMONOBJ)
TARGET =	httpproxyserver

DEPDIRS	:= 
CLEAN_DEPS := $(addprefix _clean_, $(DEPDIRS))

.PHONY: all deps clean test cleanall install $(DEPDIRS) $(TARGET)

$(TARGET):	deps $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid httpproxyserver completed!'
	@echo ''
	@echo '################################################################'

$(DEPDIRS):
	$(MAKE) -C $@
	
$(CLEAN_DEPS):	
	$(MAKE) -C $(patsubst _clean_%, %, $@) clean
				
deps:	$(DEPDIRS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid deps completed!'
	@echo ''
	@echo '################################################################'
	
all:	deps $(TARGET)
	
clean:
	rm -f $(OBJS) $(TARGET)

cleanall: clean	$(CLEAN_DEPS)
	@echo '################################################################'
	@echo ''
	@echo '# Clean all finished!'
	@echo ''
	@echo '################################################################'
	
install: 
	copy httpproxyserver.config /etc/ -rf
	copy httpproxyserver /usr/local/bin
	chmod +x /usr/local/bin/httpproxyserver
	@echo '################################################################'
	@echo ''
	@echo '# Install httpproxyserver finished!'
	@echo ''
	@echo '################################################################'
