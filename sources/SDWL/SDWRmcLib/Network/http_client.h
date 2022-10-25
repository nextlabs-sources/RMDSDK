

#ifndef __NX_REST_HTTP_CLIENT_H__
#define __NX_REST_HTTP_CLIENT_H__

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sstream>
#include <fstream>
#include "windows.h"

#include "SDLResult.h"
#include "rmccore/network/httpClient.h"
#include <nudf/filesys.hpp>

#define NXRMC_CLIENT_NAME		L"NXRMC/1.0"

namespace NX {
	class RmUploadBuffer : public std::streambuf
	{
	public:
		explicit RmUploadBuffer(const std::string& apiInput, const std::wstring& file, const std::string & boundaryend);
		virtual ~RmUploadBuffer();

		virtual void clear();

		inline size_t GetTotalDataLength() const { return _end; }
		inline size_t GetRemainDataLength() const { return (_end - _current); }

	public:
		typedef typename traits_type::int_type int_type;
		typedef typename traits_type::pos_type pos_type;
		typedef typename traits_type::off_type off_type;

	protected:
		size_t InitSize(const std::wstring& file);

	private:
		//virtual int_type overflow(int_type _Meta = traits_type::eof());
		virtual int_type pbackfail(int_type _Meta = traits_type::eof());
		virtual int_type underflow();
		virtual int_type uflow();
		//virtual pos_type seekoff(off_type _Off, std::ios_base::seekdir _Way);
		//virtual pos_type seekpos(pos_type _Pos);
		//virtual RmUploadBuffer*setbuf(char*_Buffer, std::streamsize _Count);
		//virtual int sync();
		//virtual void imbue(const std::locale& _Loc);
		virtual std::streamsize showmanyc();
		virtual std::streamsize xsgetn(char* s, std::streamsize n);
		//virtual std::streamsize xsputn(const char* s, std::streamsize n);

		// copy ctor and assignment not implemented;
		// copying not allowed
		RmUploadBuffer(const RmUploadBuffer&) = delete;
		RmUploadBuffer& operator= (const RmUploadBuffer&) = delete;

	private:
		HANDLE				_h;
		const std::string	_header;
		const std::string   _boundaryend;
		size_t				_current;
		const size_t		_end;
		bool				_feof;
	};

    namespace REST {

        namespace http {

            class Connection;
            class Request;
            class Response;
            
            class Client
            {
            public:
                Client();
                Client(const std::wstring& agent, bool ssl_enabled, unsigned long timout_seconds = 0);
                virtual ~Client();

                SDWLResult Open();
                SDWLResult Close();

                Connection* CreateConnection(const std::wstring& url, SDWLResult* res = nullptr);
                Connection* CreateConnection(const std::wstring& server, USHORT port, SDWLResult* res = nullptr);
                
                inline bool Opened() const { return (_h != nullptr); }
                inline bool Secured() const { return _ssl; }
                inline unsigned long TimeoutSeconds() const { return _tos; }
                inline PVOID GetSessionHandle() const { return _h; }

            protected:
                // No copy is allowed
                Client(const Client& rhs) {}

                SDWLResult InterOpen();

                // Callback used with WinHTTP to listen for async completions.
                static void CALLBACK HttpCompletionCallback(
                    HANDLE request_handle,
                    ULONG_PTR context,
                    unsigned long status_code,
                    void* status_info,
                    unsigned long status_info_length);

            private:
                std::wstring     _agent;
				PVOID            _h;
                bool             _ssl;
                unsigned long    _tos;
                ULONG            _ref;
                CRITICAL_SECTION _lock;
            };

            class Connection
            {
            public:
                virtual ~Connection();

                SDWLResult Connect();
                SDWLResult Disconnect();
                SDWLResult SendRequest(Request& request, Response& response, DWORD wait_ms = INFINITE, ULONG request_size = -1);

                inline bool Connected() const { return (nullptr != _h); }
                inline PVOID GetConnectHandle() const { return _h; }
                inline const std::wstring& GetServer() const { return _server; }
                inline USHORT GetPort() const { return _port; }
                inline Client* GetClient() const { return _client; }

            protected:
                // No copy is allowed
                Connection(const Connection& rhs) {}

            protected:
                Connection();
                Connection(Client* client, const std::wstring& server, USHORT port);

            private:
				PVOID			_h;
                std::wstring    _server;
				USHORT			_port;
                Client*         _client;

                friend class Client;
            };

            typedef std::vector<std::pair<std::wstring, std::wstring>> HttpHeaders;
            typedef std::vector<std::wstring> HttpAcceptTypes;

            class Request
            {
            public:
                Request() : _body_length(0), _requested_length(0), _canceled(NULL) {}
                Request(const std::wstring& method, const std::wstring& path, const HttpHeaders& headers, const HttpAcceptTypes& accept_types, bool* canceled = NULL, const ULONG requested_length = 0)
                    : _method(method), _path(path), _headers(headers), _accept_types(accept_types), _body_length(0), _requested_length(requested_length), _canceled(canceled)
                {
                }
                virtual ~Request() {}

                virtual ULONG GetRemainBodyLength() = 0;
                virtual std::istream& GetBodyStream() = 0;

				virtual  bool IsUpload() const { return false; }

				inline bool Canceled() const { return (_canceled ? (*_canceled) : false); }
                inline const std::wstring& GetMethod() const { return _method; }
                inline const std::wstring& GetPath() const { return _path; }
                inline const HttpHeaders& GetHeaders() const { return _headers; }
                inline const HttpAcceptTypes& GetAcceptTypes() const { return _accept_types; }
                inline ULONG GetBodyLength() const { return _body_length; }
                inline ULONG GetRequestedLength() const { return _requested_length; }

            protected:
                // Copy is not allowed
                Request(const Request& rhs) {}
                inline void SetBodyLength(ULONG len) { _body_length = len; }

            protected:
                std::wstring    _method;
                std::wstring    _path;
                std::vector<std::wstring> _accept_types;
                HttpHeaders     _headers;
                DWORD           _body_length;
				DWORD			_requested_length;
				bool*			_canceled;
            };

            class Response
            {
            public:
                Response() : _status(0) {}
                virtual ~Response() {}

                virtual ULONG GetBodyLength() = 0;
                virtual std::ostream& GetBodyStream() = 0;

                inline unsigned short GetStatus() const { return _status; }
                inline const std::wstring& GetPhrase() const { return _phrase; }
                inline const HttpHeaders& GetHeaders() const { return _headers; }

                inline void SetStatus(unsigned short status) { _status = status; }
                inline void SetPhrase(const std::wstring& phrase) { _phrase = phrase; }
                inline void SetHeaders(const HttpHeaders& headers) { _headers = headers; }

            protected:
                // Copy is not allowed
                Response(const Response& rhs) {}

            private:
                unsigned short  _status;
                std::wstring    _phrase;
                HttpHeaders     _headers;
            };

            class StringBodyRequest : public Request
            {
            public:
                StringBodyRequest() : Request() {}
                StringBodyRequest(const std::wstring& method, const std::wstring& path, const HttpHeaders& headers, const HttpAcceptTypes& accept_types, const std::string& body, bool* canceled = NULL, const ULONG requested_length = 0)
                    : Request(method, path, headers, accept_types, canceled, requested_length), _iss(body)
                {
                    SetBodyLength((ULONG)body.length());
                }
				StringBodyRequest(const RMCCORE::HTTPRequest& request);
                virtual ~StringBodyRequest() {}

                virtual ULONG GetRemainBodyLength()
                {
                    return (GetBodyLength() - (ULONG)_iss.tellg());
                }

                virtual std::istream& GetBodyStream()
                {
                    return _iss;
                }

            private:
                std::istringstream _iss;
            };

            class FileBodyRequest : public Request
            {
            public:
                FileBodyRequest() : Request() {}
                FileBodyRequest(const std::wstring& method, const std::wstring& path, const HttpHeaders& headers, const HttpAcceptTypes& accept_types, const std::string& file, bool* canceled = NULL, const ULONG requested_length = 0)
                    : Request(method, path, headers, accept_types, canceled, requested_length), _ifs(file, std::ifstream::in|std::ifstream::binary)
                {
                    if (_ifs.is_open()) {
                        _ifs.seekg(0, std::ios::end);
                        SetBodyLength((ULONG)_ifs.tellg());
                    }
                }
                FileBodyRequest(const std::wstring& method, const std::wstring& path, const HttpHeaders& headers, const HttpAcceptTypes& accept_types, const std::wstring& file)
                    : Request(method, path, headers, accept_types), _ifs(file, std::ifstream::in|std::ifstream::binary)
                {
                    if (_ifs.is_open()) {
                        _ifs.seekg(0, std::ios::end);
                        SetBodyLength((ULONG)_ifs.tellg());
                    }
                }
                virtual ~FileBodyRequest() {}

                virtual ULONG GetRemainBodyLength()
                {
                    return (GetBodyLength() - (ULONG)_ifs.tellg());
                }

                virtual std::istream& GetBodyStream()
                {
                    return _ifs;
                }

            private:
                std::ifstream _ifs;
            };

			class NXLUploadRequest : public Request
			{
			public:
				NXLUploadRequest(const std::wstring& method, const std::wstring& path, const HttpHeaders& headers, const HttpAcceptTypes& accept_types, const std::string& body, const std::wstring& file, const std::string& boundryend, bool* canceled = NULL, const ULONG requested_length = 0)
					:_buf(body, file, boundryend), Request(method, path, headers, accept_types, canceled, requested_length), _is(&_buf)
				{
					SetBodyLength((ULONG)_buf.GetTotalDataLength());
				}
				NXLUploadRequest(const std::wstring& method, const std::wstring& path, const HttpHeaders& headers, const HttpAcceptTypes& accept_types, const std::string& body, const std::wstring& file, const std::string& boundryend)
					: _buf(body, file, boundryend), Request(method, path, headers, accept_types), _is(&_buf)
				{
					SetBodyLength((ULONG)_buf.GetTotalDataLength());
				}
				NXLUploadRequest(const RMCCORE::HTTPRequest& request, const std::wstring& file, const std::string& boundaryend);
				virtual ~NXLUploadRequest() {}

				virtual ULONG GetRemainBodyLength()
				{
					return (ULONG)_buf.GetRemainDataLength();
				}
				virtual std::istream& GetBodyStream() { return _is; }

				virtual  bool IsUpload() const { return true; }

			protected:
				std::istream	_is;
				RmUploadBuffer	_buf;
			};

            class StringResponse : public Response
            {
            public:
                StringResponse() : Response() {}
                virtual ~StringResponse() {}

                virtual ULONG GetBodyLength()
                {
                    return (ULONG)_oss.tellp();
                }

                virtual std::ostream& GetBodyStream()
                {
                    return _oss;
                }

                std::string GetBody() const { return _oss.str(); }

            private:
                std::ostringstream _oss;
            };

            class FileResponse : public Response
            {
            public:
                FileResponse(const std::wstring& file);
				virtual ~FileResponse() {
					Finish();
					DeleteFile(m_filepath.c_str());
				}

                inline bool is_open() { return _ofs.is_open(); }

                virtual std::ostream& GetBodyStream()
                {
                    return _ofs;
                }

				void Finish() { if (is_open()) { _ofs.close(); } }

            private:
                virtual ULONG GetBodyLength()
                {
                    return 0;
                }

            private:
                std::ofstream _ofs;
				const std::wstring & m_filepath;
            };

        }
    }
}

#endif