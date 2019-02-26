/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*-
 *
 * rcscript.h
 * Copyright (C) 2005 Brainzsquare, Inc.
 *
 * 2005/11/21 <changmin@brainz.co.kr>
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#include "rcscript.h"
#include "snprintf.h"

// CRcScript
//CRcScript::CRcScript(char* pszFileName)
// 2008.04.07 <lkh05@brainz.co.kr> - CJ
CRcScript::CRcScript(char* pszFileName, char* pszInstallPath)
{
	const char* initPath[] = {
		"/etc/rc.d/init.d",
		"/etc/init.d",
		"/sbin/init.d",
		NULL
	};
	
	const char* rcPath[] = {
		"/etc/rc.d",
		"/etc",
		"/sbin",
		NULL
	};

	for (int i = 0; initPath[i]; i++) {
		if (!access(initPath[i], 0)) {
			memset(m_szPathName, 0, sizeof m_szPathName);
			strncpy(m_szPathName, rcPath[i], sizeof m_szPathName - 1);
			memset(m_szLinkName, 0, sizeof m_szLinkName);
			strncpy(m_szLinkName, rcPath[i], sizeof m_szLinkName - 1);
			break;
		}
	}

	// SUSE Linux
	if (!access("/etc/init.d/rc0.d", 0)) {
		strncpy(m_szLinkName, "/etc/init.d", sizeof m_szPathName - 1);
	}

	SetFileName(pszFileName);
	SetInstallPath(pszInstallPath);		// 2008.04.07 <lkh05@brainz.co.kr> - CJ
	SetUserName((char*) "root");
}

CRcScript::~CRcScript() { }

char* CRcScript::GetPathName()
{
	return m_szPathName;
}

void CRcScript::Destroy(CRcScript *pScript)
{
	if(pScript) delete pScript;
}

CRcScript* CRcScript::Create(char* pszFileName, char* pszInstallPath, 
		char* pszUserName)
{
	CRcScript* rcScript = NULL;
	if (pszFileName && pszInstallPath) {
		struct utsname u;
		if (uname(&u) >= 0) {
			if (strstr(u.sysname, "Linux"))
				rcScript = new CRcScriptLinux(pszFileName, pszInstallPath);
			else if (strstr(u.sysname, "SunOS"))
				rcScript = new CRcScriptSolaris(pszFileName, pszInstallPath);
			else if (strstr(u.sysname, "UnixWare"))
				rcScript = new CRcScriptSolaris(pszFileName, pszInstallPath);
			else if (strstr(u.sysname, "HP-UX"))
				rcScript = new CRcScriptHpux(pszFileName, pszInstallPath);
			else if (strstr(u.sysname, "OSF1"))
				rcScript = new CRcScriptOsf1(pszFileName, pszInstallPath);
			else if (strstr(u.sysname, "AIX"))
				rcScript = new CRcScriptAix(pszFileName, pszInstallPath);
			else if (strstr(u.sysname, "FreeBSD"))
				rcScript = new CRcScriptFreeBSD(pszFileName, pszInstallPath);
		}
		// added by kskim. 2008.7.16
		if(rcScript && pszUserName) {
			rcScript->SetUserName(pszUserName);
		}
	}
	return rcScript;
}

char* CRcScript::GetLinkName()
{
	return m_szLinkName;
}

char* CRcScript::GetFileName()
{
	return m_szFileName;
}

void CRcScript::SetFileName(char* pszFileName)
{
	if (pszFileName && strlen(pszFileName)) {
		memset(m_szFileName, 0, sizeof m_szFileName);
		strncpy(m_szFileName, pszFileName, sizeof m_szFileName - 1);
	}
}

char* CRcScript::GetInstallPath()
{
	return m_szInstallPathName;
}

void CRcScript::SetInstallPath(char* pszPath)
{
	if (pszPath && strlen(pszPath)) {
		memset(m_szInstallPathName, 0, sizeof m_szInstallPathName);
		strncpy(m_szInstallPathName, pszPath, sizeof m_szInstallPathName - 1);
	}
}

void CRcScript::Install()
{
	char tmpFile[512];
	memset(tmpFile, 0, sizeof tmpFile);
	mysnprintf(tmpFile, sizeof tmpFile - 1,
			 "%s/init.d/%s_rcscript_tmp",
			 GetPathName(), GetFileName());

	if (!WriteScript(tmpFile)) {
		unlink(tmpFile);
		return;
	}

	char rcFile[512];
	memset(rcFile, 0, sizeof rcFile);
	mysnprintf(rcFile, sizeof rcFile - 1,
			 "%s/init.d/%s", GetPathName(), GetFileName());
	unlink(rcFile);
	rename(tmpFile, rcFile);
	chmod(rcFile, 0755);
}

void CRcScript::Uninstall()
{
	char rcFile[512];
	memset(rcFile, 0, sizeof rcFile);
	mysnprintf(rcFile, sizeof rcFile - 1,
			 "%s/init.d/%s", GetPathName(), GetFileName());
	unlink(rcFile);
}

void CRcScript::Remove()
{
	if (strlen(GetFileName())) {
		char varPath[512];
		memset(varPath, 0, sizeof varPath);
		mysnprintf(varPath, sizeof varPath - 1,
				"/var/%s", GetFileName());
		if (!access(varPath, 0)) {
			char rmCmd[512];
			memset(rmCmd, 0, sizeof rmCmd);
			mysnprintf(rmCmd, sizeof rmCmd - 1, "/bin/rm -rf %s", varPath);
			chdir("/tmp");
			system(rmCmd);
		}
	}
}

void CRcScript::LinkScript(int runLevel, int mode)
{
	char srcFile[512];
	memset(srcFile, 0, sizeof srcFile);
	if (strstr(GetLinkName(), "init.d"))
		mysnprintf(srcFile, sizeof srcFile - 1,
				 "%s/%s",
				GetLinkName(), GetFileName());
	else
		mysnprintf(srcFile, sizeof srcFile - 1,
				 "%s/init.d/%s",
				GetLinkName(), GetFileName());

	char linkFile[512];
	if (mode == AllScript || mode == StartScript) {
		memset(linkFile, 0, sizeof linkFile);
		mysnprintf(linkFile, sizeof linkFile - 1,
#if		_HPUX
				 "%s/rc%d.d/S999%s",
#else
				 "%s/rc%d.d/S99%s",
#endif
				 GetLinkName(), runLevel, GetFileName());
		unlink(linkFile);

		fprintf(stderr, "Link %s %s\n", srcFile, linkFile);
#if		_SOLARIS 
		if (link(srcFile, linkFile) < 0)
			perror("link");
#else
		symlink(srcFile, linkFile);
#endif
	}
	
	if (mode == AllScript || mode == KillScript) {
		memset(linkFile, 0, sizeof linkFile);
		mysnprintf(linkFile, sizeof linkFile - 1,
#if		_HPUX
				 "%s/rc%d.d/K100%s",
#else
				 "%s/rc%d.d/K10%s",
#endif
				 GetLinkName(), runLevel, GetFileName());
		unlink(linkFile);
#if		_SOLARIS 
		if (link(srcFile, linkFile) < 0)
			perror("link");
#else
		symlink(srcFile, linkFile);
#endif
	}
}

void CRcScript::UnlinkScript(int runLevel)
{
	char linkFile[512];
	memset(linkFile, 0, sizeof linkFile);
	mysnprintf(linkFile, sizeof linkFile - 1,
#if		_HPUX
			 "%s/rc%d.d/S999%s",
#else
			 "%s/rc%d.d/S99%s",
#endif
			 GetLinkName(), runLevel, GetFileName());
	unlink(linkFile);
	
	memset(linkFile, 0, sizeof linkFile);
	mysnprintf(linkFile, sizeof linkFile - 1,
#if		_HPUX
			 "%s/rc%d.d/K100%s",
#else
			 "%s/rc%d.d/K10%s",
#endif
			 GetLinkName(), runLevel, GetFileName());
	unlink(linkFile);
}

// CRcScriptLinux
CRcScriptLinux::CRcScriptLinux(char* pszFileName, char* pszInstallPath) : CRcScript(pszFileName, pszInstallPath) { }

CRcScriptLinux::~CRcScriptLinux() { }

void CRcScriptLinux::Install()
{
	CRcScript::Install();
	for (int runLevel = 2; runLevel < 6; runLevel++) {
		LinkScript(runLevel);
	}
}

void CRcScriptLinux::Uninstall()
{
	CRcScript::Uninstall();
	for (int runLevel = 2; runLevel < 6; runLevel++) {
		UnlinkScript(runLevel);
	}
}

bool CRcScriptLinux::WriteScript(char* pszFileName)
{
	if (!pszFileName || !strlen(pszFileName))
		return false;
	
	FILE* fp = fopen(pszFileName, "w");
	if (!fp)
		return false;

	fprintf(fp, "#!/bin/sh\n\n");
	fprintf(fp, "# chkconfig: 2345 99 10\n");
	fprintf(fp, "# probe: true\n\n");
	//fprintf(fp, "[ -f /usr/sbin/%s ] || exit 0\n\n", GetFileName());
	// 2008.04.07 <lkh05@brainz.co.kr> - CJ
	fprintf(fp, "[ -f %s/%s ] || exit 0\n\n", GetInstallPath(), GetFileName());
	fprintf(fp, "RETVAL=0\n\n");
	fprintf(fp, "start() {\n");
	fprintf(fp, "    echo -n \"Starting %s: \"\n", GetFileName());
	//fprintf(fp, "    /usr/sbin/%s -start >& /dev/null\n", GetFileName());
	// 2008.04.07 <lkh05@brainz.co.kr> - CJ
	fprintf(fp, "    su - %s -c \"%s/%s -start\" >& /dev/null\n",
			GetUserName(), GetInstallPath(), GetFileName());
	fprintf(fp, "    echo\n");
	fprintf(fp, "}\n\n");
	fprintf(fp, "stop() {\n");
	fprintf(fp, "    echo -n \"Shutting down %s: \"\n", GetFileName());
	fprintf(fp, "    su - %s -c \"%s/%s -stop\"\n", GetUserName(), GetInstallPath(), GetFileName());
	fprintf(fp, "    sleep 1\n");
	fprintf(fp, "    su - %s -c \"%s/%s -kill\"\n", GetUserName(), GetInstallPath(), GetFileName());
	fprintf(fp, "    echo\n");
	fprintf(fp, "}\n\n");
	fprintf(fp, "kill() {\n");
	fprintf(fp, "    su - %s -c \"%s/%s -kill\"\n", GetUserName(), GetInstallPath(), GetFileName());
	fprintf(fp, "}\n\n");
	fprintf(fp, "restart() {\n");
	fprintf(fp, "    stop\n");
	fprintf(fp, "    sleep 1\n");
	fprintf(fp, "    kill\n");
	fprintf(fp, "    start\n");
	fprintf(fp, "}\n\n");
	fprintf(fp, "case \"$1\" in\n");
	fprintf(fp, "    start)\n");
	fprintf(fp, "        start\n");
	fprintf(fp, "        ;;\n");
	fprintf(fp, "    stop)\n");
	fprintf(fp, "        stop\n");
	fprintf(fp, "        ;;\n");
	fprintf(fp, "    restart)\n");
	fprintf(fp, "        restart\n");
	fprintf(fp, "        ;;\n");
	fprintf(fp, "    *)\n");
	fprintf(fp, "                echo \"Usage: %s {start|stop|restart}\"\n",
			GetFileName());
	fprintf(fp, "        exit 1\n");
	fprintf(fp, "esac\n\n");
	fprintf(fp, "exit $RETVAL\n");
	fclose(fp);
	
	return true;
}

// CRcScriptSolaris
CRcScriptSolaris::CRcScriptSolaris(char* pszFileName, char* pszInstallPath) : CRcScript(pszFileName, pszInstallPath) { }

CRcScriptSolaris::~CRcScriptSolaris() { }

void CRcScriptSolaris::Install()
{
	CRcScript::Install();
	LinkScript(3);
}

void CRcScriptSolaris::Uninstall()
{
	CRcScript::Uninstall();
	UnlinkScript(3);
}

bool CRcScriptSolaris::WriteScript(char* pszFileName)
{
	if (!pszFileName || !strlen(pszFileName))
		return false;
	
	FILE* fp = fopen(pszFileName, "w");
	if (!fp)
		return false;

	fprintf(fp, "#!/sbin/sh\n");
	fprintf(fp, "#\n");
	fprintf(fp, "# Copyright (c) 2000-2005 by Brainzsquare Inc.\n");
	fprintf(fp, "# All rights reserved.\n");
	fprintf(fp, "#\n");
	fprintf(fp, "# description: Start or stop the %s daemon\n",
			GetFileName());
	fprintf(fp, "#\n");
	fprintf(fp, "#ident  \"@(#)%s     2.40    09/09/2002\"\n",
			GetFileName());
	fprintf(fp, "\n");
	fprintf(fp, "case \"$1\" in\n");
	fprintf(fp, "'start')\n");
	fprintf(fp, "    echo \"Starting %s: \"\n", GetFileName());
	// 2008.04.07 <lkh05@brainz.co.kr> - CJ	
	fprintf(fp, "    su - %s -c \"%s/%s -start &\"\n",
			GetUserName(), GetInstallPath(), GetFileName());
	fprintf(fp, "    ;;\n");
	fprintf(fp, "\n");
	fprintf(fp, "'stop')\n");
	fprintf(fp, "    echo \"Stop %s: \"\n", GetFileName());
	//fprintf(fp, "    /usr/sbin/%s -stop\n", GetFileName());
	//fprintf(fp, "    /usr/sbin/%s -kill\n", GetFileName());
	// 2008.04.07 <lkh05@brainz.co.kr> - CJ	
	fprintf(fp, "    su - %s -c \"%s/%s -stop\"\n", GetUserName(), GetInstallPath(), GetFileName());
	fprintf(fp, "    su - %s -c \"%s/%s -kill\"\n", GetUserName(), GetInstallPath(), GetFileName());
	fprintf(fp, "    ;;\n");
	fprintf(fp, "\n");
	fprintf(fp, "*)\n");
	fprintf(fp, "    echo \"Usage: $0 { start | stop }\"\n");
	fprintf(fp, "    exit 1\n");
	fprintf(fp, "    ;;\n");
	fprintf(fp, "esac\n");
	fprintf(fp, "exit 0\n");
	fclose(fp);
	
	return true;
}

// CRcScriptHpux
CRcScriptHpux::CRcScriptHpux(char* pszFileName, char* pszInstallPath) : CRcScript(pszFileName, pszInstallPath) { }

CRcScriptHpux::~CRcScriptHpux() { }

void CRcScriptHpux::Install()
{
	CRcScript::Install();
	LinkScript(2, KillScript);
	LinkScript(3, StartScript);
}

void CRcScriptHpux::Uninstall()
{
	CRcScript::Uninstall();
	UnlinkScript(2);
	UnlinkScript(3);
}

bool CRcScriptHpux::WriteScript(char* pszFileName)
{
	if (!pszFileName || !strlen(pszFileName))
		return false;
	
	FILE* fp = fopen(pszFileName, "w");
	if (!fp)
		return false;

	fprintf(fp, "#!/sbin/sh\n");
	fprintf(fp, "#\n");
	fprintf(fp, "#\n");
	fprintf(fp, "#           Copyright (C) 2000-2005 Brainzsquare, Inc.\n");
	fprintf(fp, "#\n");
	fprintf(fp, "# NOTE:    This script is not configurable! Any changes made to this\n");
	fprintf(fp, "#          script will be overwritten when you upgrade to the next release.\n");
	fprintf(fp, "#\n\n");
	//fprintf(fp, "PATH=/sbin:/usr/sbin:/usr/bin\n");
	// 2008.04.07 <lkh05@brainz.co.kr> - CJ	
	fprintf(fp, "PATH=/sbin:%s:/usr/bin\n", GetInstallPath());
	fprintf(fp, "export PATH\n\n");
	fprintf(fp, "rval=0\n");
	fprintf(fp, "set_return() {\n");
	fprintf(fp, "        x=$?\n");
	fprintf(fp, "        if [ $x -ne 0 ]; then\n");
	fprintf(fp, "                echo \"EXIT CODE: $x\"\n");
	fprintf(fp, "                rval=1  # always 1 so that 2 can be used for other reasons\n");
	fprintf(fp, "        fi\n");
	fprintf(fp, "}\n\n");
	fprintf(fp, "case \"$1\" in\n");
	fprintf(fp, "start_msg)\n");
	fprintf(fp, "        echo \"Start Zenius Agent daemon(%s)\"\n", GetFileName());
	fprintf(fp, "        ;;\n\n");
	fprintf(fp, "stop_msg)\n");
	fprintf(fp, "        echo \"Stopping Zenius Agent daemon(%s)\"\n", GetFileName());
	fprintf(fp, "        ;;\n\n");
	fprintf(fp, "'start')\n");
	fprintf(fp, "        if [ -f /etc/rc.config ]; then\n");
	fprintf(fp, "                . /etc/rc.config\n");
	fprintf(fp, "        else\n");
	fprintf(fp, "                echo \"ERROR: /etc/rc.config defaults file MISSING\"\n");
	fprintf(fp, "        fi\n\n");
	fprintf(fp, "        mask=`umask`\n");
	fprintf(fp, "        umask 000\n\n");
	fprintf(fp, "        [ -x %s/%s ] && su - %s -c \"%s/%s -start\"\n",
			GetInstallPath(), GetFileName(), GetUserName(), GetInstallPath(), GetFileName());
	fprintf(fp, "        if [ $? -eq 0 ]; then\n");
	fprintf(fp, "                echo \"Zenius Agent started\"\n");
	fprintf(fp, "        else\n");
	fprintf(fp, "                echo \"Unable to start Zenius Agent\"\n");
	fprintf(fp, "        fi\n\n");
	fprintf(fp, "        umask $mask\n");
	fprintf(fp, "        ;;\n\n");
	fprintf(fp, "'stop')\n");
	fprintf(fp, "        if [ -f /etc/rc.config ]; then\n");
	fprintf(fp, "                . /etc/rc.config\n");
	fprintf(fp, "        else\n");
	fprintf(fp, "                echo \"ERROR: /etc/rc.config defaults file MISSING\"\n");
	fprintf(fp, "        fi\n\n");
	fprintf(fp, "        set_return\n");
	fprintf(fp, "        [ -x %s/%s ] && su - %s -c \"%s/%s -stop\"\n",
			GetInstallPath(), GetFileName(), GetUserName(), GetInstallPath(), GetFileName());
	fprintf(fp, "        [ -x %s/%s ] && su - %s -c \"%s/%s -kill\"\n",
			GetInstallPath(), GetFileName(), GetUserName(), GetInstallPath(), GetFileName());
	fprintf(fp, "        if [ $rval -eq 0 ]; then\n");
	fprintf(fp, "                echo \"Zenius Agent stopped\"\n");
	fprintf(fp, "        else\n");
	fprintf(fp, "                echo \"Unable to stop Zenius Agent\"\n");
	fprintf(fp, "        fi\n");
	fprintf(fp, "        ;;\n\n");
	fprintf(fp, "*)\n");
	fprintf(fp, "        echo \"usage: $0 {start|stop}\"\n");
	fprintf(fp, "        rval=1\n");
	fprintf(fp, "        ;;\n");
	fprintf(fp, "esac\n\n");
	fprintf(fp, "exit $rval\n");
	fclose(fp);
	
	return true;
}

// CRcScriptOsf1
CRcScriptOsf1::CRcScriptOsf1(char* pszFileName, char* pszInstallPath) : CRcScript(pszFileName, pszInstallPath) { }

CRcScriptOsf1::~CRcScriptOsf1() { }

void CRcScriptOsf1::Install()
{
	CRcScript::Install();
	LinkScript(0, KillScript);
	LinkScript(2, StartScript);
	LinkScript(3, StartScript);
}

void CRcScriptOsf1::Uninstall()
{
	CRcScript::Uninstall();
	UnlinkScript(0);
	UnlinkScript(2);
	UnlinkScript(3);
}

bool CRcScriptOsf1::WriteScript(char* pszFileName)
{
	if (!pszFileName || !strlen(pszFileName))
		return false;
	
	FILE* fp = fopen(pszFileName, "w");
	if (!fp)
		return false;

	fprintf(fp, "#!/sbin/sh\n\n");
	//fprintf(fp, "AGENT=/usr/sbin/%s\n", GetFileName());
	//fprintf(fp, "PATH=/sbin:/usr/sbin:/usr/bin\n");
	// 2008.04.07 <lkh05@brainz.co.kr> - CJ	
	fprintf(fp, "AGENT=%s/%s\n", GetInstallPath(), GetFileName());
	fprintf(fp, "PATH=/sbin:%s:/usr/bin\n", GetInstallPath());
	fprintf(fp, "case \"$1\" in\n");
	fprintf(fp, "'start')\n");
	fprintf(fp, "    su - %s -c \"$AGENT -start\"> /dev/console 2>&1 &\n",
			GetUserName());
	fprintf(fp, "    echo \"%s started\"\n", GetFileName());
	fprintf(fp, "        ;;\n\n");
	fprintf(fp, "'stop')\n");
	fprintf(fp, "    su - %s -c \"$AGENT -stop\"\n", GetUserName());
	fprintf(fp, "    ;;\n\n");
	fprintf(fp, "*)\n");
	fprintf(fp, "        echo \"usage: $0 {start|stop}\"\n");
	fprintf(fp, "        ;;\n");
	fprintf(fp, "esac\n");
	fclose(fp);
	
	return true;
}

// CRcScriptAix
CRcScriptAix::CRcScriptAix(char* pszFileName, char* pszInstallPath) : CRcScript(pszFileName, pszInstallPath) { }

CRcScriptAix::~CRcScriptAix() { }

void CRcScriptAix::Install()
{
	char cmdBuf[512];
	memset(cmdBuf, 0, sizeof cmdBuf);
	mysnprintf(cmdBuf, sizeof cmdBuf - 1,
		 "/usr/bin/cat /etc/inittab | /usr/bin/sed '/%s/d' > /etc/inittab.%s",
		 GetFileName(), GetFileName());
	system(cmdBuf);

	memset(cmdBuf, 0, sizeof cmdBuf);
#ifdef _KSKIM
	mysnprintf(cmdBuf, sizeof cmdBuf - 1,
			 "/usr/bin/echo \"%s:2:once:%s/%s -start\" >> /etc/inittab.%s",
			 GetFileName(), GetInstallPath(), GetFileName(), GetFileName());
#else
	mysnprintf(cmdBuf, sizeof cmdBuf - 1,
			 "/usr/bin/echo \"%s:2:once:/usr/bin/su - %s -c \"%s/%s -start\"\" >> /etc/inittab.%s",
			 GetFileName(), GetUserName(), GetInstallPath(), GetFileName(), GetFileName());
#endif
	system(cmdBuf);

	memset(cmdBuf, 0, sizeof cmdBuf);
	mysnprintf(cmdBuf, sizeof cmdBuf - 1,
			 "/usr/bin/mv /etc/inittab.%s /etc/inittab",
			 GetFileName());
	system(cmdBuf);
}

void CRcScriptAix::Uninstall()
{
	char cmdBuf[512];
	memset(cmdBuf, 0, sizeof cmdBuf);
	mysnprintf(cmdBuf, sizeof cmdBuf - 1,
			 "/usr/bin/cat /etc/inittab | /usr/bin/sed '/%s/d' > /etc/inittab.%s",
			 GetFileName(), GetFileName());
	system(cmdBuf);
	
	memset(cmdBuf, 0, sizeof cmdBuf);
	mysnprintf(cmdBuf, sizeof cmdBuf - 1,
			 "/usr/bin/mv /etc/inittab.%s /etc/inittab",
			 GetFileName());
	system(cmdBuf);
}

bool CRcScriptAix::WriteScript(char* pszFileName)
{
	return true;
}
