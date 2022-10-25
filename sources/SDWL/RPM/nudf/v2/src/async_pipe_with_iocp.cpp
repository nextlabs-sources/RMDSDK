#include <nudf/async_pipe_with_iocp.hpp>
#include <nudf/eh.hpp>
#include <nudf/debug.hpp>
#include <nudf/winutil.hpp>

static const std::wstring pipe_name_prefix(L"\\\\.\\pipe\\");
const unsigned int pending_connects = 10;

NX::async_pipe_with_iocp::server::server():_buffer_size(4096), _timeout(3000), _pending_connects(0),
_number_of_concurrent_thread(0) // will using default value 0 (0 means the number of concurrent thrad equals the number of cpu)
{
}

NX::async_pipe_with_iocp::server::server(unsigned long size, unsigned long timeout, unsigned int concurrent_thread):_buffer_size(size),
_timeout(timeout), _number_of_concurrent_thread(concurrent_thread), _pending_connects(0)
{
}

NX::async_pipe_with_iocp::server::~server() 
{
	shutdown();
}

bool NX::async_pipe_with_iocp::server::listen(const std::wstring& port) 
{
	_pipe_name = pipe_name_prefix + port;

	// Create the main worker thread
	DWORD thread_id = 0;
	_thread_main = CreateThread(NULL,
		0,
		(LPTHREAD_START_ROUTINE)main_thread,
		this,
		0, // the thread run immediately after create.
		&thread_id);
	if (NULL == _thread_main) {
		return false;
	}

	return true;
}

void NX::async_pipe_with_iocp::server::shutdown() 
{
	if (_iocp == nullptr) {
		return;
	}

	_shutdown_event.set();

	WaitForSingleObject(_thread_connect, INFINITE);
	CloseHandle(_thread_connect);

	PostQueuedCompletionStatus(_iocp, 0, (ULONG_PTR)0, NULL);

	WaitForSingleObject(_thread_main, INFINITE);
	CloseHandle(_thread_main);

	CloseHandle(_iocp);

	clean_up();
}

void NX::async_pipe_with_iocp::server::remove_overlapped(myoverlapped* polp)
{
	std::lock_guard<std::mutex> lock(_mutex_olp);

	auto count = _vector_overlapped.size();
	for (std::size_t i = 0; i < count; ++i)
	{
		if (polp == _vector_overlapped.at(i))
		{
			DisconnectNamedPipe(polp->pipe());
			CloseHandle(polp->pipe());

			delete polp;

			_vector_overlapped.erase(_vector_overlapped.begin() + i);

			break;
		}
	}
}

void NX::async_pipe_with_iocp::server::read(myoverlapped* polp) noexcept
{
	bool write_response = false;
	unsigned long read_bytes = polp->bytes_available();
	if (read_bytes != 0) {
		on_read(polp->buffer(), &read_bytes, &write_response);
	}
	// No data from client or error occurs
	else {
		write_response = false;
	}

	if (write_response) {
		assert(0 != read_bytes);
		polp->set_bytes_available(read_bytes);

		DWORD write_bytes = 0;
		if (WriteFile(polp->pipe(),
			polp->buffer(),
			read_bytes,
			&write_bytes, // the para can be NULL for async io operation
			&polp->_overlapped))
		{
			PostQueuedCompletionStatus(_iocp,
				write_bytes,
				(ULONG_PTR)polp->pipe(),
				&polp->_overlapped);
		}

		// means the async IO requst failed to add into the driver queue.
		else if (GetLastError() != ERROR_IO_PENDING) {
			write_response = false;
		}
	}

	if (!write_response) {
		// Probably pipe is being closed, or pipe not connected
		remove_overlapped(polp);
	}

}

void NX::async_pipe_with_iocp::server::clean_up()
{
	std::lock_guard<std::mutex> lock(_mutex_olp);
	auto size = _vector_overlapped.size();
	for (auto& one : _vector_overlapped) {
		CancelIo(one->pipe());
		CloseHandle(one->pipe());
	}

	_vector_overlapped.clear();
	_pending_connects = 0;
}

// This method should be override by sub-class
void NX::async_pipe_with_iocp::server::on_read(unsigned char* data, unsigned long* len, bool* write_response) 
{
	*len = 0;
	*write_response = false;
}

DWORD WINAPI NX::async_pipe_with_iocp::server::main_thread(LPVOID lpvod) 
{
	server* pserv = static_cast<server*>(lpvod);
	
	// Create a new IO completion port
	if ((pserv->_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0)) == NULL) {
		return 0;
	}

	DWORD thread_id;
	pserv->_thread_connect = CreateThread(NULL,
		0,
		(LPTHREAD_START_ROUTINE)connect_thread,
		pserv,
		0,
		&thread_id);


	ULONG_PTR*		ppre_handle_key = 0;
	OVERLAPPED*		poverlapped;
	myoverlapped*	pmyoverlapped;
	DWORD           transfered_bytes = 0;
	BOOL			ret;

	while (true) {

		// Continually loop to listen IO completion, current thread is pending until
		// the IO operation completion.
		ret = GetQueuedCompletionStatus(pserv->_iocp,
			&transfered_bytes,
			(PULONG_PTR)&ppre_handle_key,
			&poverlapped,
			INFINITE);

		// In shutdown method, called "PostQueuedCompletionStatus" and passed 0 as the completion key.
        // will trigger the while loop break and the thread exit cleanly.
		if (ppre_handle_key == NULL || poverlapped == NULL) {
			break; 
		}

		pmyoverlapped = CONTAINING_RECORD(poverlapped, myoverlapped, _overlapped);

		switch (pmyoverlapped->op_code())
		{
		case completion_key::CK_CONNECT: 
			{
				InterlockedDecrement(&pserv->_pending_connects);
				pserv->_more_connects_event.set();

				pmyoverlapped->set_op_code(CK_READ);

				DWORD bytes_read = 0;
				if (!ReadFile(pmyoverlapped->pipe(),
					pmyoverlapped->buffer(),
					pmyoverlapped->buffer_size(),
					&bytes_read, // the param can be NULL for async io
					&pmyoverlapped->_overlapped)) 
				{ // For async io
					DWORD error = GetLastError();
					// the async io request failed to add into the driver queue.
					// maybe the pipe disconnected by client
					if (error != ERROR_IO_PENDING && error != ERROR_PIPE_LISTENING) {
						pserv->remove_overlapped(pmyoverlapped);
						break;
					}
				}
				else 
				{ // For sync io
					pmyoverlapped->set_bytes_available(bytes_read);

					PostQueuedCompletionStatus(pserv->_iocp,
						bytes_read,
						(ULONG_PTR)pmyoverlapped->pipe(),
						&pmyoverlapped->_overlapped);
				}

			}
			break;
		
		case completion_key::CK_READ:
			{ // read complete
				

			    // No data from pipe client or read error, disconnect pipe.
				if (transfered_bytes == 0) {
					pmyoverlapped->set_op_code(CK_DISCONNECT);
					break;
				}

				// handle the read data then write back
				pmyoverlapped->set_op_code(CK_WRITE);
				pmyoverlapped->set_bytes_available(transfered_bytes);

				pair_t* param = new pair_t(pserv, pmyoverlapped);

				HANDLE thread;
				DWORD tid = 0;
				if ((thread = CreateThread(NULL,  
					0,
					(LPTHREAD_START_ROUTINE)instance_thread,
					param,
					0,
					&tid)))
				{
					CloseHandle(thread);
				}
			}
			break;

		case completion_key::CK_WRITE:
			{ // write complete

			    // check make sure all the data has been send
				assert(transfered_bytes == pmyoverlapped->bytes_available());

				// set op-code and wait for client to read data written the disconnect.
			    pmyoverlapped->set_op_code(CK_DISCONNECT); 
			}
			break;

		case completion_key::CK_DISCONNECT:
			{
				DisconnectNamedPipe(pmyoverlapped->pipe());
				pserv->remove_overlapped(pmyoverlapped);
			}
			break;
		}

	} // while

	return 0;
}

DWORD WINAPI NX::async_pipe_with_iocp::server::connect_thread(LPVOID lpvod) 
{
	server* pserv = static_cast<server*>(lpvod);

	HANDLE arr_handles[2];
	arr_handles[0] = pserv->_shutdown_event.get_event();
	arr_handles[1] = pserv->_more_connects_event.get_event();

	DWORD wait_ret;
	HANDLE hpipe;
	myoverlapped* polp;

	do {

		{
			std::lock_guard<std::mutex> lock(pserv->_mutex_olp);

			while (pserv->_pending_connects < pending_connects)
			{
				static const unsigned long open_mode = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED;
				static const unsigned long pipe_mode = PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE;

				NX::win::security_attribute sa(NX::win::explicit_access(SECURITY_WORLD_SID_AUTHORITY, SECURITY_WORLD_RID,
					GENERIC_READ | GENERIC_WRITE, SUB_CONTAINERS_AND_OBJECTS_INHERIT));

				// Create the pipe instance
				hpipe = ::CreateNamedPipe(pserv->_pipe_name.c_str(),
					open_mode,
					pipe_mode,
					PIPE_UNLIMITED_INSTANCES,
					pserv->_buffer_size,
					pserv->_buffer_size,
					pserv->_timeout,
					&sa);
				if (INVALID_HANDLE_VALUE == hpipe) {
					break;
				}

				// bind the new pipe instance to the completion port.
				CreateIoCompletionPort(hpipe, 
					pserv->_iocp, 
					(ULONG_PTR)hpipe,  // Using pipe handle as completion key
					pserv->_number_of_concurrent_thread);

				polp = new myoverlapped(hpipe, CK_CONNECT, pserv->_buffer_size);
				if (!polp) {
					CloseHandle(hpipe);
					break;
				}

				if (!ConnectNamedPipe(hpipe, &polp->_overlapped)) 
				{
					DWORD last_error = ::GetLastError();
					if (last_error == ERROR_PIPE_CONNECTED) {
						pserv->_vector_overlapped.push_back(polp);

						PostQueuedCompletionStatus(pserv->_iocp, 0, (ULONG_PTR)hpipe, &polp->_overlapped);
						continue;
					}
					// connected then disconnected before ConnectNamedPipe called
					else if (last_error == ERROR_NO_DATA) {
						delete polp;

						DisconnectNamedPipe(hpipe);
						CloseHandle(hpipe);

						continue;
					}
					else if (last_error != ERROR_PIPE_LISTENING && last_error != ERROR_IO_PENDING) {
						delete polp;
						CloseHandle(hpipe);
						continue;
					}
				} // ConnectNamedPipe

				// io pending status
				pserv->_vector_overlapped.push_back(polp);
				++pserv->_pending_connects;

			} // while

		} // lock_guard

		wait_ret = ::WaitForMultipleObjects(2, arr_handles, FALSE, INFINITE);
		pserv->_more_connects_event.reset();

	} while (wait_ret != WAIT_OBJECT_0);

	return 0;
}

DWORD WINAPI NX::async_pipe_with_iocp::server::instance_thread(LPVOID lpvod) 
{
	pair_t* p = static_cast<pair_t*>(lpvod);

	p->first->read(p->second);
	delete p;

	return 0;
}

// lpEventAttributes: NULL
// bManualReset: TRUE
// bInitialState: FALSE (nonsignaled)
// lpName: NULL
NX::async_pipe_with_iocp::myevent::myevent():_hEvent(CreateEvent(NULL, TRUE, FALSE, NULL))
{
}

NX::async_pipe_with_iocp::myevent::myevent(LPSECURITY_ATTRIBUTES lpSecurityAttr, bool bManualRest, bool bInitState):
	_hEvent(CreateEvent(lpSecurityAttr, bManualRest, bInitState, NULL))
{
}

NX::async_pipe_with_iocp::myevent::myevent(LPSECURITY_ATTRIBUTES lpSecurityAttr, bool bManualRest, bool bInitState, const wchar_t * pName):
	_hEvent(CreateEvent(lpSecurityAttr, bManualRest, bInitState, pName))
{
}

NX::async_pipe_with_iocp::myevent::~myevent()
{
	::CloseHandle(_hEvent);
}

HANDLE NX::async_pipe_with_iocp::myevent::get_event() const
{
	return _hEvent;
}

bool NX::async_pipe_with_iocp::myevent::wait() const
{
	if (!wait(INFINITE)) {
		return false;
	}

	return true;
}

bool NX::async_pipe_with_iocp::myevent::wait(unsigned long timeout) const
{
	bool bRet = true;

	DWORD dwRet = ::WaitForSingleObject(_hEvent, timeout);
	if (dwRet == WAIT_OBJECT_0) {
		bRet = true;
	}
	else if (dwRet == WAIT_TIMEOUT) {
		bRet = false;
	}

	return bRet;
}

void NX::async_pipe_with_iocp::myevent::reset()
{
	if (!ResetEvent(_hEvent)) {
		throw NX::exception(WIN32_ERROR_MSG2(::GetLastError()));
	}
}

void NX::async_pipe_with_iocp::myevent::set()
{
	if (!SetEvent(_hEvent)) {
		throw NX::exception(WIN32_ERROR_MSG2(::GetLastError()));
	}
}

void NX::async_pipe_with_iocp::myevent::pulse()
{
	if (!PulseEvent(_hEvent)) {
		throw NX::exception(WIN32_ERROR_MSG2(::GetLastError()));
	}
}
