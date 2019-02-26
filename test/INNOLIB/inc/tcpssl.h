/*
 * TcpSSL 2.0
 * Copyright (c) 2004 Brainzsquare, Inc.
 */

////////////////////////
// Secure Socket

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif

#ifndef __TCPSSL_H__
#define __TCPSSL_H__

// SSL Header File
#include <openssl/rsa.h>       /* SSLeay stuff */
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/ui.h>

#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/rc4.h>

#include <openssl/bio.h>

#ifndef _WIN32
#include <pthread.h>
#endif
#include "tcpsock.h"

#define SSL_CLIENT  0
#define SSL_SERVER  1

//! SSL 로 암호와하여 전송하기 위한 Client TCP 소켓 클래스 
class CTcpSSL : public CTcpSock
{
public:
	SSL_METHOD* m_meth;
	SSL_CTX*    m_ctx;
	SSL*        m_ssl;
	int			m_systype;

public:
    CTcpSSL(const char* pHost=NULL,Port nPort=80);
    virtual ~CTcpSSL();

	int 	InitSSL(const char* cert_file, const char* key_file);
	void	FinishSSL(); 	// 2009.09.02 hjson

	void	SetSysType(int nType);
	int 	SSLConnect();
	int 	SSLAccept(SSL_CTX* pCtx);

	// override
	int 	Send(char* msg,int size,int opt=0);
	int 	SendAsync(char* msg,int size,int opt=0);

	// override
	int 	Recv(char* msg,int size,int opt=0);
	int 	RecvAsync(char* msg,int size,int opt=0);

	// override
	void 	Close();
};

#endif
