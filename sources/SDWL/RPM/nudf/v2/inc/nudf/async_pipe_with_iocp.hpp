#ifndef __ASYNC_PIPE_WITH_IOCP_HPP__
#define __ASYNC_PIPE_WITH_IOCP_HPP__

#include <Windows.h>
#include <vector>
#include <memory>
#include <mutex>

static const unsigned long default_buffer_size = 4096;

namespace NX {
	/*
	  The "async_pipe_with_iocp" is a asynchronous named pipeline server endpoint extension of "async_pipe" in asyncpope.hpp.
	  Since the latter accepts the notification of async IO request completion by using alertable IO, which biggest problem is that the
	  thread sending the IO requests and the thread processing the results must be the same one, when processing a time-consuming 
	  operation of the IO request from one pipe client, the subsequent other IO requests may not be processed or can't connect to the 
	  pipe server.

	  In order to resolve the above issue, we extend the async pipe server using IO completion port(IOCP) technology.
	*/
namespace async_pipe_with_iocp {

	enum  completion_key 
	{
		CK_CONNECT,
		CK_READ,
		CK_WRITE,
		CK_DISCONNECT
	};

	class myevent
	{
	public:
		myevent();
		myevent(LPSECURITY_ATTRIBUTES lpSecurityAttr, bool bManualRest, bool bInitState);
		myevent(LPSECURITY_ATTRIBUTES lpSecurityAttr, bool bManualRest, bool bInitState, const wchar_t* pName);
		virtual ~myevent();

		HANDLE get_event() const;
		bool wait() const;
		bool wait(unsigned long timeout) const;
		void reset();
		void set();
		void pulse();

	private:
		HANDLE _hEvent;
	};

	class myoverlapped;
	class server
	{
	public:
		server();
		server(unsigned long size, unsigned long timeout = 3000, unsigned int concurrent_thread = 0);
		virtual ~server();

		bool listen(const std::wstring& port);
		void shutdown();

		virtual void on_read(unsigned char* data, unsigned long* length, bool* write_response);

	private:
		using vec_overlapped_t = std::vector<myoverlapped*>;
		using pair_t = std::pair<server*, myoverlapped*>;

		static DWORD WINAPI main_thread(LPVOID lpvd);
		static DWORD WINAPI connect_thread(LPVOID lpvd);
		static DWORD WINAPI instance_thread(LPVOID lpvd);

		void remove_overlapped(myoverlapped* polp);
		void read(myoverlapped* pOlp) noexcept;
		void clean_up();

	private:
		myevent _shutdown_event;
		myevent _more_connects_event;

		std::wstring _pipe_name;
		unsigned long _buffer_size;
		unsigned long _timeout;
		unsigned int _number_of_concurrent_thread;

		HANDLE _iocp;

		HANDLE _thread_main;
		HANDLE _thread_connect;
		HANDLE _thread_instance;

		std::mutex _mutex_olp;
		vec_overlapped_t _vector_overlapped;
		unsigned int _pending_connects;
	};

	class  myoverlapped
	{
	public:
		myoverlapped() :_pipe(INVALID_HANDLE_VALUE), _op_code(0), _bytes_available(0) {
			memset(&_overlapped, 0, sizeof(_overlapped));
			_buffer.resize(default_buffer_size, 0);
		}

		myoverlapped(HANDLE h, unsigned int op_code, unsigned long buffer_size) :_pipe(h), _op_code(op_code),
			_bytes_available(0) {
			memset(&_overlapped, 0, sizeof(_overlapped));
			_buffer.resize(buffer_size, 0);
		}

		~myoverlapped() {
			_buffer.clear();
		}

		inline unsigned long bytes_available() const noexcept {
			return _bytes_available;
		}

		inline void set_bytes_available(unsigned long bytes) noexcept {
			_bytes_available = bytes;

			// make sure data is swiped
			if (_bytes_available < (unsigned long)_buffer.size()) {
				memset(&_buffer[_bytes_available], 0, (unsigned long)_buffer.size() - _bytes_available);
			}
		}

		inline unsigned char* buffer() {
			return _buffer.empty() ? nullptr : _buffer.data();
		}

		inline unsigned long buffer_size() const {
			return (unsigned long)_buffer.size();
		}

		inline unsigned int op_code() const noexcept {
			return _op_code;
		}

		inline void set_op_code(unsigned int code) noexcept {
			_op_code = code;
		}

		inline HANDLE pipe() const noexcept {
			return _pipe;
		}

		OVERLAPPED _overlapped;

	private:
		HANDLE _pipe;
		unsigned int _op_code;

		// transfered bytes
		unsigned long _bytes_available;

		std::vector<unsigned char> _buffer;
	};

} // namespace NX::async_pipe_with_iocp

} // namespace NX


#endif // !__ASYNC_PIPE_WITH_IOCP_HPP__

