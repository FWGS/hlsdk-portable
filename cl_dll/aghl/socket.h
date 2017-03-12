// socket.h

#ifndef SOCKET_H
#define SOCKET_H

#pragma comment(lib, "wsock32.lib")

#include <winsock.h>


class WinsockInit
	{
	public :
		WSADATA m_wsd;
		int m_iStatus;

		WinsockInit(WORD wVersionRequested = 0x0101);
		~WinsockInit();
	};


class InetAddr : public sockaddr_in
	{
	public :
		InetAddr(WORD wPort = 0);
		InetAddr(LPCSTR lpszAddress, WORD wPort = 0);
		InetAddr& operator = (LPCSTR lpszAddress);

	protected :
		void Resolve(LPCSTR lpszAddress, WORD wPort = 0);
	};


class Socket
	{
	public :
		Socket();
		Socket(SOCKET s);
		Socket(const Socket& s);
		virtual ~Socket();

		bool Create();
		void Close();
		bool Bind(const InetAddr& addr);
		bool Connect(const InetAddr& addr);
		bool Listen();
		Socket Accept(InetAddr& addr);
		int Send(const unsigned char* buf, int cbBuf);
		int Send(const char* fmt, ...);
		int Receive(unsigned char* buf, int cbBuf);
		bool SetOpt(int opt, const char* pBuf, int cbBuf);
		bool GetOpt(int opt, char* pBuf, int& cbBuf);
		operator SOCKET& () const { return (SOCKET&)m_sock; }
		operator bool() const { return m_sock != INVALID_SOCKET; }

	protected :
		SOCKET m_sock;

	private :
		bool m_bOwnSocket;
	};


#endif
