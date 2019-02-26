#ifndef _PACKET_H
#define _PACKET_H


class CPacket
{
    public:
        CPacket();
        ~CPacket();
        

        void InitPacket(int client_sockfd);
        void InitPacket(int client_sockfd, int logflag);
        void InitPacket(int category, int command, int client_sockfd);
        void InitPacket(int category, int command, int client_sockfd, int logflag);
        

        int ReadTinyInt(int &intvalue);
        int ReadShortInt(int &intvalue);
        int ReadInt(int &intvalue);
        int ReadBigInt(long long &longvalue);
        int ReadChar(char *charvalue, int size);
        int ReadShortString(char *stringvalue, int maxsize);
        int ReadString(char *stringvalue, int maxsize);
    

        int WriteTinyInt(int intvalue);
        int WriteShortInt(int intvalue);
        int WriteInt(int intvalue);
        int WriteBigInt(long long longvalue);
        int WriteChar(char *charvalue, int size);
        int WriteShortString(char *stringvalue);
        int WriteString(char *stringvalue);
    

        int IsEOD();
        

        int ReceivePacket();
        int ReceivePacket_Select();

        int SendPacket();

        int m_nCategory;
        int m_nCommand;

    protected:

        void CheckDataBuffer(int size);
        

        int CheckDataExists(int size);
        
    private:
        int m_nClientSockfd;
        int m_nDataLength;
        int m_nDataPointer;
        char *m_strData;
        int m_logflag;
};

#endif //_PACKET_H


