//A simple UDP client.

#pragma comment(lib,"wsock32.lib")

class AgUDPClient
{
public:
  static bool Init()
  {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(1,1), &wsaData))
      return false;
    return true;
  }
  
  static bool Cleanup()
  {
    if (WSACleanup())
      return false;
    return true;
  }

  AgUDPClient()
  {
    m_Socket = INVALID_SOCKET;
    ZeroMemory(&m_ServerAddress,sizeof(m_ServerAddress));
  }
  
  bool Connect(const char* pszAddress, unsigned short usPort)
  {
    m_Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_Socket == INVALID_SOCKET)
      return false;
    
    m_ServerAddress.sin_family = AF_INET;
    m_ServerAddress.sin_port = htons(usPort);
    m_ServerAddress.sin_addr.s_addr = inet_addr(pszAddress);
    
    if (m_ServerAddress.sin_addr.s_addr == INADDR_NONE)
    {
      //Resolve hostname.
      struct hostent* host = gethostbyname(pszAddress);
      if (host == NULL)
        return false; //Could not resolve hostname
      CopyMemory(&m_ServerAddress.sin_addr,
        host->h_addr_list[0],host->h_length);
    }
    
    if (connect(m_Socket,(struct sockaddr *)&m_ServerAddress, sizeof(m_ServerAddress)) == SOCKET_ERROR)
      return false;
    
    return true;
  }
  
  bool WaitForRecieve(UINT uiMilliSeconds = INFINITE)
  {
    fd_set ReadSet;
    FD_ZERO(&ReadSet);
    FD_SET(m_Socket, &ReadSet);
    if (1 != select(0, &ReadSet, NULL, NULL, TimeVal(uiMilliSeconds)))
      return false;
    return true;
  }
  
  bool WaitForSend(UINT uiMilliSeconds = INFINITE)
  {
    fd_set WriteSet;
    FD_ZERO(&WriteSet);
    FD_SET(m_Socket, &WriteSet);
    if (1 != select(0, NULL, &WriteSet, NULL, TimeVal(uiMilliSeconds)))
      return false;
    return true;
  }
  
  bool WaitForError(UINT uiMilliSeconds = INFINITE)
  {
    fd_set ErrorSet;
    FD_ZERO(&ErrorSet);
    FD_SET(m_Socket, &ErrorSet);
    if (1 != select(0, NULL, NULL, &ErrorSet, TimeVal(uiMilliSeconds)))
      return false;
    return true;
  }
  
  unsigned long Send(const char* pData, const unsigned long lSize)
  {
    return send(m_Socket, pData, lSize, 0);
  }
  
  unsigned long Receive(char* pData, const unsigned long lSize)
  {
    return recv(m_Socket, pData, lSize, 0);
  }
  
protected:
  class TimeVal
  {
  public:
    TimeVal(UINT uiMilliSeconds)
    {
      m_bNull = false;
      if (uiMilliSeconds == INFINITE)
        m_bNull = true;
      else if(uiMilliSeconds <= 0)
      {
        m_TimeVal.tv_sec  = 0;
        m_TimeVal.tv_usec = 0;
      }
      else
      {
        m_TimeVal.tv_sec  = uiMilliSeconds / 1000;
        m_TimeVal.tv_usec = (uiMilliSeconds % 1000) * 1000;
      }
    }
    
    operator timeval*()
    {
      if (m_bNull)
        return NULL;
      return &m_TimeVal;
    }
  protected:
    bool m_bNull;
    timeval m_TimeVal;
  };
  
  SOCKET m_Socket;
  struct sockaddr_in m_ServerAddress;
};
