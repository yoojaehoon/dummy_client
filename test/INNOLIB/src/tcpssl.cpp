/*
 * TcpSSL 2.0
 * Copyright (c) 2004 Brainzsquare, Inc.
 */

#include "stdafx.h"

#ifdef _WIN32
#include <time.h>
#else
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/time.h>
#endif

#include "mutext.h"
#include "tcpssl.h"

static const char rnd_seed[] = "string to make the random number generator think it has entropy";

#define CHK_SSL(err) if((err) == -1) { ERR_print_errors_fp(stderr); }

//////////////////////////
// CTcpSSL - Client

static CMutext *g_pMutCrypto; 	// 2009.07.16 hjson
static int      g_nMutCrypto; 	// 2009.07.16 hjson

/* SSL multithread locking callback */
static void SSLLockCallback(int mode, int n, const char *file, int line) 
{
	if (n >= 0 && n < g_nMutCrypto) {
		if (mode & CRYPTO_LOCK) {
			g_pMutCrypto[n].Lock();
		} else {
			g_pMutCrypto[n].Unlock();
		}
	} else {
		fprintf(stderr, "libssl::locking_callback() error!");
	}
}

/* SSL thread ID callback */
static unsigned long SSLIdCallback() 
{
	unsigned long id = 0;
#ifdef _WIN32
	id = (unsigned long)GetCurrentThreadId();
#else
	id = (unsigned long)pthread_self();
#endif
	return id;
}

CTcpSSL::CTcpSSL(const char* svr, Port svr_prt) : CTcpSock(svr,svr_prt)
{
	m_meth 	= NULL;
	m_ctx 	= NULL;
	m_ssl 	= NULL;
	m_systype = SSL_CLIENT;
}

CTcpSSL::~CTcpSSL()
{
	Close();
}

int CTcpSSL::InitSSL(const char* cert_file, const char* key_file)
{
	int nRet = 0;

	SSL_load_error_strings();
	SSLeay_add_ssl_algorithms();

	// session mutex 2009.07.15 hjson
	g_nMutCrypto = CRYPTO_num_locks();
	if(g_nMutCrypto>0 && g_pMutCrypto==NULL) {
		fprintf(stderr,"SSL: max connections [%d]\n",g_nMutCrypto);
		g_pMutCrypto = new CMutext[g_nMutCrypto];
		CRYPTO_set_locking_callback(SSLLockCallback);
		CRYPTO_set_id_callback(SSLIdCallback);
	}

	if(m_systype==SSL_SERVER && cert_file && key_file) { // 2009.09.02 hjson
		RAND_seed(rnd_seed,strlen(rnd_seed)); 	// 2009.07.15 hjson

		//m_meth = SSLv23_server_method();
		m_meth = SSLv3_server_method();

		m_ctx = SSL_CTX_new (m_meth);
		if(m_ctx == NULL) {
			fprintf(stderr, "SSL_CTX_new Fail\n");
			return nRet;
		}
		//SSL_CTX_set_timeout(m_ctx,GetTimeout());

		if(SSL_CTX_use_certificate_file(m_ctx,cert_file,SSL_FILETYPE_PEM)<=0) {
			fprintf(stderr, "error: setting certificate\n");
			return nRet;
		}
    	if(SSL_CTX_use_PrivateKey_file(m_ctx,key_file,SSL_FILETYPE_PEM)<=0) {
			fprintf(stderr,"error: setting private key\n");
			return nRet;
		}
		if(!SSL_CTX_check_private_key(m_ctx)) {
			fprintf(stderr,"Private key does not match the public key\n");
			return nRet;
		}

		// cipher
		SSL_CTX_set_cipher_list(m_ctx,"AES256-SHA"); // 2009.06.24 hjson
		nRet = 1; 	// 2009.09.03 hjson
	}

	return nRet;
}

void CTcpSSL::FinishSSL()
{
	CRYPTO_set_id_callback(NULL);
	CRYPTO_set_locking_callback(NULL);
	if(g_pMutCrypto) {
		delete[] g_pMutCrypto;
		g_pMutCrypto = NULL;
	}
}

void CTcpSSL::SetSysType( int nType )
{
	m_systype = nType; // SSL_CLIENT : 0, SSL_SERVER : 1
}

int CTcpSSL::SSLConnect()
{
    int	nRet = -1;
	if(Socket() < 0) {
		Close();
		return -1;
	}

	m_meth = SSLv3_client_method(); 
	m_ctx = SSL_CTX_new(m_meth);
	if(m_ctx == NULL) {
		fprintf(stderr, "SSL_CTX_new fail\n");
		Close();
		return -1;
	}

#ifndef _WIN32	// 2010.01.15 hjson Windows에서 오작동 
	// socket timeout 2009.06.26 hjson
	struct timeval tv;
	int ret;
	tv.tv_sec = GetTimeout();
	tv.tv_usec = 0;
	ret = setsockopt(Socket(),SOL_SOCKET,SO_RCVTIMEO,(char*)&tv,sizeof(tv));
	if(ret == -1) fprintf(stderr,"Socket SO_RCVTIMEO failed\n");
	ret = setsockopt(Socket(),SOL_SOCKET,SO_SNDTIMEO,(char*)&tv,sizeof(tv));
	if(ret == -1) fprintf(stderr,"Socket SO_SNDTIMEO failed\n");
#endif

	//SSL_CTX_set_timeout(m_ctx,GetTimeout());
	m_ssl = SSL_new(m_ctx);
	if(m_ssl) {
		SSL_clear(m_ssl); // 2009.07.15 hjson
		SSL_set_fd(m_ssl,Socket());
		SSL_set_connect_state(m_ssl); 	// 2009.09.02 hjson
		nRet = SSL_connect(m_ssl);
	}
	if(nRet <= 0) Close(); 	// 2009.07.14 hjson

	return nRet;
}

int CTcpSSL::SSLAccept(SSL_CTX* pCtx)
{
	int nRet = -1;
	int timeout = GetTimeout();
	int fd = Socket();

	if(fd <= 0 || pCtx == NULL) {
		fprintf(stderr,"SSLAccept failed: fd [%d] or SSL_CTX [NULL]\n",fd);
		Close();
		return nRet;
	}
	if(timeout < 5) timeout = 5; // 2009.09.01 hjson

#ifndef _WIN32 // 2010.01.15 hjson Windows에서 오작동
	// socket timeout 2009.06.26 hjson
	struct timeval tv;
	int ret;
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	ret = setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,(char*)&tv,sizeof(tv));
	if(ret == -1) fprintf(stderr,"Socket SO_RCVTIMEO failed\n");
	ret = setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,(char*)&tv,sizeof(tv));
	if(ret == -1) fprintf(stderr,"Socket SO_SNDTIMEO failed\n");
#endif

	struct timeval to;
	fd_set rfd;
	int nfd;
	FD_ZERO(&rfd);
	FD_SET(fd,&rfd);
	to.tv_sec = timeout;
	to.tv_usec = 0;
	nfd = select(fd+1,&rfd,NULL,NULL,&to);
	if(nfd > 0) {
		m_ssl = SSL_new(pCtx);
		if(m_ssl) {
			SSL_clear(m_ssl);
			SSL_set_fd(m_ssl,fd);
			SSL_set_accept_state(m_ssl); 	// 2009.09.02 hjson
			nRet = SSL_accept(m_ssl);
		}
	}
	if(nRet <= 0) {
		fprintf(stderr,"failed to accept SSL\n");
		Close(); 	// 2009.07.14 hjson
	}

	return nRet;
}

int CTcpSSL::Send(char* msg,int size,int opt)
{
	int nwritten, nleft = size;
	while(m_ssl && nleft>0) {
		// write
		nwritten = SSL_write(m_ssl,msg,nleft);
		if(nwritten <= 0) {
			Close();
			return nwritten;
		}
		nleft -= nwritten;
		msg += nwritten;
	}

	return (size-nleft);
}

int CTcpSSL::SendAsync(char* msg,int size,int opt)
{
	int nwritten = -1;
	if(m_ssl) nwritten = SSL_write(m_ssl,msg,size);
	if(nwritten <= 0) Close();
	return nwritten;
}

int CTcpSSL::Recv(char* msg,int size,int opt)
{
	int nread, nleft = size;
	memset(msg,0,size);
	while(m_ssl && nleft>0) {
		// read
		nread = SSL_read(m_ssl,msg,nleft);
		if(nread <= 0) {
			Close();
			return nread;
		}
		nleft -= nread;
		msg += nread;
	}

	if(nleft > 0) return 0;
	return size;
}

int CTcpSSL::RecvAsync(char* msg,int size,int opt)
{
	// initialize memory
	memset(msg,0,size);
	int nread = -1;
	if(m_ssl) nread = SSL_read(m_ssl,msg,size);
	if(nread <= 0) Close();
	return nread;
}

void CTcpSSL::Close()
{
	if(m_ssl) {
		// commented 2009.07.14 hjson
		//SSL_set_shutdown(m_ssl,SSL_SENT_SHUTDOWN|SSL_RECEIVED_SHUTDOWN);
		SSL_free(m_ssl);
		m_ssl = NULL;
	}
	// 2009.07.14 hjson
	CTcpSock::Close(); 
	// free context
	if(m_ctx) {
		SSL_CTX_free(m_ctx);
		m_ctx = NULL;
	}
}

#ifdef _TCPSSL_TEST

int main()
{
	CTcpSSL sock;
	char buf[4096];
	int ret;

	sock.InitSSL(NULL,NULL);
	ret = sock.Connect("10.0.0.199",5042);
	printf("Connect [%d]\n",ret);
	ret = sock.SSLConnect();
	printf("SSLConnect [%d]\n",ret);
	ret = sock.Recv(buf,sizeof(buf));
	printf("SSLRead [%d]\n",ret);

	return 0;
}

#endif
