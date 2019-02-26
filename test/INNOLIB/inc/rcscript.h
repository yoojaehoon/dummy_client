/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*-
 *
 * rcscript.h
 * Copyright (C) 2005 Brainzsquare, Inc.
 *
 * 2008/07/14 <kskim@brainz.co.kr> - 
 * 변수명, 클래스명, 수정 함. 생성/파괴방식수정(Create function)
 * 2005/11/21 <changmin@brainz.co.kr>
 */

#ifndef _RC_SCRIPT_H
#define _RC_SCRIPT_H

class CRcScript {
public:
	enum {
		AllScript	= 0,
		StartScript	= 1,
		KillScript	= 2
	};

private:
	char m_szUserName[128]; 		// 2008.07.16 <kskim@brainz.co.kr>
	char m_szPathName[512];
	char m_szLinkName[512];
	char m_szFileName[512];
	char m_szInstallPathName[512];	// 2008.04.07 <lkh05@brainz.co.kr> - CJ

public:
	//CRcScript(char* pszFileName = NULL);
	// 2008.04.07 <lkh05@brainz.co.kr> - CJ
	CRcScript(char* pszFileName = NULL, char* pszInstallPath = NULL);
	virtual ~CRcScript();

	//static CRcScript* Create(char* pszFileName);
	// 2008.04.07 <lkh05@brainz.co.kr> - CJ
	// 반드시 Destroy를 호출해주어야한다.
	static CRcScript* Create(char* pszFileName, char* pszInstallPath, char* pszUser = NULL);
	// added by kskim. 2008.07.16
	static void Destroy(CRcScript *pScript);
	virtual void Install();
	virtual void Uninstall();
	virtual void Remove();

	// added by kskim. 2008.7.16
	void  SetUserName(char* pszUser) { strcpy(m_szUserName, pszUser); }

	virtual bool WriteScript(char* pszFileName) = 0;

protected:
	// added by kskim. 2008.7.16
	char* GetUserName() { return m_szUserName; }
	char* GetPathName();
	char* GetLinkName();
	char* GetFileName();
	void  SetFileName(char* pszFileName);

	// 2008.04.07 <lkh05@brainz.co.kr> - CJ
	char* GetInstallPath();
	void  SetInstallPath(char* pszInstallPath);

	void LinkScript(int runLevel, int mode = AllScript);
	void UnlinkScript(int runLevel);
};

class CRcScriptLinux : public CRcScript {
public:
	//CRcScriptLinux(char* pszFileName);
	// 2008.04.07 <lkh05@brainz.co.kr> - CJ
	CRcScriptLinux(char* pszFileName, char* pszInstallPath);
	virtual ~CRcScriptLinux();
	
	void Install();
	void Uninstall();

private:
	bool WriteScript(char* pszFileName);
};

class CRcScriptSolaris : public CRcScript {
public:
	//CRcScriptSolaris(char* pszFileName);
	// 2008.04.07 <lkh05@brainz.co.kr> - CJ
	CRcScriptSolaris(char* pszFileName, char* pszInstallPath);
	virtual ~CRcScriptSolaris();
	
	void Install();
	void Uninstall();

private:
	bool WriteScript(char* pszFileName);
};

class CRcScriptHpux : public CRcScript {
public:
	//CRcScriptHpux(char* pszFileName);
	// 2008.04.07 <lkh05@brainz.co.kr> - CJ
	CRcScriptHpux(char* pszFileName, char* pszInstallPath);
	virtual ~CRcScriptHpux();
	
	void Install();
	void Uninstall();

private:
	bool WriteScript(char* pszFileName);
};

class CRcScriptOsf1 : public CRcScript {
public:
	//CRcScriptOsf1(char* pszFileName);
	// 2008.04.07 <lkh05@brainz.co.kr> - CJ
	CRcScriptOsf1(char* pszFileName, char* pszInstallPath);
	virtual ~CRcScriptOsf1();
	
	void Install();
	void Uninstall();

private:
	bool WriteScript(char* pszFileName);
};

class CRcScriptAix : public CRcScript {
public:
	//CRcScriptAix(char* pszFileName);
	// 2008.04.07 <lkh05@brainz.co.kr> - CJ
	CRcScriptAix(char* pszFileName, char* pszInstallPath);
	virtual ~CRcScriptAix();
	
	void Install();
	void Uninstall();

private:
	bool WriteScript(char* pszFileName);
};

#endif	// _RC_SCRIPT_H
