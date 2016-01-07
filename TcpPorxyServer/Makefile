# Copyright (C) 2015 The VncHttpProxyServer
# TcpProxyServer Makefile
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
CXXFLAGS +=	-I. -Ilibev -Iinclude

LIBS =		-L. \
			-Wl,-Bstatic -Llibev/.libs -lev \
			-Wl,-Bdynamic -ldl -lz -lpthread 

COMMONOBJ =	common/LogFile.o common/md5.o common/KThread.o \
			common/ConfFile.o common/Arithmetic.o
TASKOBJ = 	task/BaseTask.o task/ProxyClientTask.o 
OBJS =		server.o TcpProxyServer.o TcpServer.o MessageList.o LogManager.o \
			DataParser.o Client.o Session.o
OBJS +=		$(COMMONOBJ)
OBJS +=		$(TASKOBJ)
TARGET =	tcpproxyserver

DEPDIRS	:= libev
CLEAN_DEPS := $(addprefix _clean_, $(DEPDIRS))

.PHONY: all deps clean test cleanall install $(DEPDIRS) $(TARGET)

$(TARGET):	deps $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)
	@echo '################################################################'
	@echo ''
	@echo '# Bulid tcpproxyserver completed!'
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
	copy tcpproxyserver.config /etc/ -rf
	copy tcpproxyserver /usr/local/bin
	chmod +x /usr/local/bin/tcpproxyserver
	@echo '################################################################'
	@echo ''
	@echo '# Install tcpproxyserver finished!'
	@echo ''
	@echo '################################################################'
