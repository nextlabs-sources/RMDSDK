
#pragma once

namespace NX {

	namespace win {

		class HostInfo
		{
		public:
			HostInfo();
			HostInfo(const HostInfo& rhs);
			virtual ~HostInfo();

			HostInfo& operator = (const HostInfo& rhs);

			static HostInfo QueryLocalHostInfo();

			inline bool Empty() const { return _hostName.empty(); }
			inline bool InDomain() const { return (!_domainName.empty()); }

			inline const std::wstring& GetHostName() const { return _hostName; }
			inline const std::wstring& GetDomainName() const { return _domainName; }
			inline const std::wstring& GetFQDN() const { return _fqdn; }

		protected:
		private:
			std::wstring    _hostName;
			std::wstring    _domainName;
			std::wstring    _fqdn;
		};

		// Since Host Information won't change after system start
		// We keep a global value here for optmization
		const HostInfo& GetLocalHostInfo();
	}	// NX::win

}   // NX

