#ifndef _H_NETWORK_WIN
#define _H_NETWORK_WIN

#include "Common.h"
#include "Thread.h"
#include "NetworkPacket.h"

namespace AGK
{
	class AGKSocket;

	class AGKSocketTimeout : public AGKThread
	{
		protected:
			AGKSocket *m_pSocket;
			uint32_t m_iTimeout;

			uint32_t Run();

		public:
			AGKSocketTimeout();
			~AGKSocketTimeout();

			void SetData( AGKSocket *pSocket, uint32_t iTimeout );
	};

	class AGKSocket : public AGKThread
	{
		private:
			friend class AGKSocketTimeout;

		public:
			AGKSocket *m_pNext;

		protected:
			SOCKET m_client;
			char m_szIP[ 65 ];
			uint32_t m_port;
			volatile bool m_bConnected;
			volatile bool m_bConnecting;
			volatile bool m_bDisconnected;
			uint32_t m_iTimeout;
			bool m_bASync;

			volatile float m_fProgress;
			volatile bool m_bResult;
			
			char m_sendBuffer[ 1400 ];
			uint32_t m_iSendWritePtr;

			AGKSocketTimeout m_cTimeout;
						
			uint32_t Run();
			void Reset();
			int IsIPV6() { return strchr(m_szIP,':') ? 1 : 0; }

		public:

			AGKSocket();
			AGKSocket( SOCKET s );
			~AGKSocket();

			const char *GetRemoteIP() { return m_szIP; }

			bool Flush();
			void Close( bool bGraceful=true );
			void ForceClose();
			bool GetDisconnected() { return m_bDisconnected; }

			bool Connect( const char* IP, uint32_t port, uint32_t timeout=3000 );
			bool ConnectASync( const char* IP, uint32_t port, uint32_t timeout=3000 );
			bool IsConnected() { return m_bConnected; }
			bool IsConnecting() { return m_bConnecting; }
			float GetProgress() { return m_fProgress; }

			void SetTimeout( int iLength );
			
			bool SendFile( const char* szFilename );
			bool SendData( const char* s, int length );
			bool SendString( const char *s );
			bool SendChar( char c );
			bool SendUInt( uint32_t u );
			bool SendInt( int i );
			bool SendFloat( float f );

			// if this is zero the recv commands will block
			int GetBytes();
			
			int		RecvData( char* data, int length ); // blocks
			int		RecvString( uString &s );			// blocks
			char	RecvChar();							// blocks
			int		RecvInt();							// blocks
			uint32_t	RecvUInt();							// blocks
			float	RecvFloat();						// blocks			
	};

	class UDPManager
	{
		protected:
			SOCKET m_socket;
			uint32_t m_port;
			int m_iIPv6;
			int m_iValid;

		public:
			UDPManager( const char* szIP, uint32_t listenPort );
			~UDPManager();
			int IsValid() { return m_iValid; }

			bool SendPacket( const char *IP, uint32_t port, const AGKPacket *packet ); // does not block
			bool RecvPacket( char *fromIP, int *fromPort, AGKPacket *packet );  // does not block
			bool PacketReady();  // does not block
	};

	// will listen for new connections until stopped
	class cNetworkListener : public AGKThread
	{
		protected:
			SOCKET m_socket;
			uint32_t m_port;
			cLock m_lock;

			AGKSocket* volatile m_pConnections;

			uint32_t Run();

		public:
			cNetworkListener();
			~cNetworkListener();

			AGKSocket* GetNewConnection();
			int AcceptConnections( const char *szIP, uint32_t port );
			int AcceptConnections( uint32_t port );
			void Stop();
	};

	// will wait forever for broadcast packets until Stop() is called
	class BroadcastListener
	{
		protected:
			SOCKET m_socket;
			sockaddr_storage addr;
			//char m_data[1500];
			//char m_from[45];
			//void (*callback)(int, const char*, const char*);

			//uint32_t Run();

		public:
			BroadcastListener();
			~BroadcastListener();

			void Close();

			bool SetListenPort( const char *szIP, uint32_t port );
			bool SetListenPort( uint32_t port );
			bool ReceivedBroadcast();
			bool GetPacket( AGKPacket &packet, uint32_t &fromPort, char *fromIP );
	};

	// will broadcast a packet a regular intervals until stopped.
	class Broadcaster : public AGKThread
	{
		protected:
			//char m_data[1400];
			AGKPacket m_packet;
			//uint32_t m_size;
			uint32_t m_interval;
			int m_max;
			uint32_t m_port;
			int m_ipv6; // IPv6 or IPv4

			uint32_t Run();

		public:

			Broadcaster();
			~Broadcaster() {};

			// sets the data that will be broadcast every interval milliseconds for a maximum of max broadcasts (0=forever)
			// start with Start() and stop with Stop()
			//void SetData( uint32_t port, const char* buf, uint32_t length, uint32_t interval, int max=0 );
			void SetData( int ipv6, uint32_t port, const AGKPacket *packet, uint32_t interval, int max=0 );
	};

	class cHTTPHeader
	{
		public:
			uString sName;
			uString sValue;

			cHTTPHeader() {}
			~cHTTPHeader() {}
	};

	class cHTTPConnection : public AGKThread
	{
		protected:
			static HINTERNET m_hInet;

			char *m_sURL;
			HINTERNET m_hInetConnect;
			int m_iSecure;
			volatile bool m_bConnected;
			int m_iTimeout;
			int m_iVerifyMode;
			
			char* volatile m_szResponse;
			float volatile m_fProgress;
			int volatile m_iStatusCode;
			char m_szContentType[150];

			bool m_bSaveToFile;
			uString m_szServerFile;
			uString m_szLocalFile;
			uString m_szUploadFile;
			uString m_szPostData;
			uString m_sUsername;
			uString m_sPassword;

			cHashedList<cHTTPHeader*> m_cHeaders;

			char* SendRequestInternal();
			char* SendFileInternal();
			uint32_t Run();

		public:
			cHTTPConnection();
			~cHTTPConnection();

			void Stop();

			bool SetHost( const char *szHost, int iSecure, const char *szUser=0, const char *szPass=0 );
			void Close();
			void SetTimeout( int milliseconds );
			void SetVerifyCertificate( int mode );

			void AddHeader( const char* headerName, const char* headerValue );
			void RemoveHeader( const char* headerName );

			float GetProgress() { return m_fProgress; }
			int GetStatusCode() { return m_iStatusCode; }

			// caller must delete returned pointer
			char* SendRequest( const char *szServerFile, const char *szPostData=0 ); //blocks
			bool SendRequestASync( const char *szServerFile, const char *szPostData=0 ); //does not block
			bool SendFile( const char *szServerFile, const char *szPostData, const char *szLocalFile ); //does not block

			int GetResponseReady();
			const char* GetResponse(); // caller must NOT delete returned pointer
			const char* GetContentType();

			bool DownloadFile( const char *szServerFile, const char *szLocalFile, const char *szPostData=0 ); //does not block
			bool DownloadComplete();
	};
}

#endif