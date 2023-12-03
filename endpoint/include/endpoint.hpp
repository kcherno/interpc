#ifndef ENDPOINT_HPP
#define ENDPOINT_HPP

#include <stdexcept>

extern "C" {

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
    
}

namespace endpoint {
    class endpoint_interface;
    class net_endpoint;
    class tcp_endpoint;
}

class endpoint::endpoint_interface {
public:
    using address_t = struct ::sockaddr;

    constexpr endpoint_interface() noexcept : identifier {-1} {}
    
    explicit constexpr endpoint_interface(int id)
    {
	if (id < 0)
	    throw std::runtime_error {::strerrorname_np(errno)};

	identifier = id;
    }
    
    endpoint_interface(const endpoint_interface&) = delete;
    
    constexpr endpoint_interface(endpoint_interface&& endpoint) noexcept
    {
	endpoint_interface::operator=(std::move(endpoint));
    }

    endpoint_interface& operator=(const endpoint_interface&) = delete;

    constexpr endpoint_interface&
    operator=(endpoint_interface&& endpoint) noexcept
    {
	identifier          = endpoint.identifier;
	endpoint.identifier = -1;

	return *this;
    }
    
    virtual ~endpoint_interface()
    {
	close();
    }

    virtual void create()  = 0;
    virtual void bind()    = 0;
    virtual void connect() = 0;
    
    virtual const address_t* address() const noexcept = 0;
    
    
    void listen(int backlog) const
    {
	if (::listen(identifier, backlog) == -1)
	    throw std::runtime_error {::strerrorname_np(errno)};
    }

    void close()
    {
	if (identifier >= 0) {
	    if (::close(identifier) == -1)
		throw std::runtime_error {::strerrorname_np(errno)};

	    identifier = -1;
	}
    }

    constexpr int fd() const noexcept
    {
	return identifier;
    }
    
protected:
    void create(int domain, int type, int protocol)
    {
	close();
	
	int retfd = ::socket(domain, type, protocol);
	
	if (retfd == -1)
	    throw std::runtime_error {::strerrorname_np(errno)};
	
	identifier = retfd;
    }

    void bind(const address_t* addr, socklen_t addrlen) const
    {
	if (::bind(identifier, addr, addrlen) == -1)
	    throw std::runtime_error {::strerrorname_np(errno)};
    }
    
    void connect(const address_t* addr, socklen_t addrlen) const
    {
	if (::connect(identifier, addr, addrlen) == -1)
	    throw std::runtime_error {::strerrorname_np(errno)};
    }
    
private:
    int identifier;
};

class endpoint::net_endpoint : public endpoint::endpoint_interface {
public:
    using address_t = struct ::sockaddr_in;

    constexpr net_endpoint() noexcept : endpoint_interface() {}
    
    explicit constexpr net_endpoint(int id) : endpoint_interface(id) {}

    net_endpoint(int id, std::string_view sp, uint16_t port) : endpoint_interface(id)
    {
	address_t taddr;
	
	::memset(&taddr, 0, sizeof(taddr));
	
	taddr.sin_family = AF_INET;
	taddr.sin_port   = ::htons(port);
	
	if (::inet_aton(sp.data(), &taddr.sin_addr) == 0)
	    throw std::runtime_error {"Invalid IPv4 address format"};

	::memcpy(&addr, &taddr, sizeof(addr));
    }

    net_endpoint(const net_endpoint&) = delete;

    constexpr net_endpoint(net_endpoint&& endpoint) noexcept
    {
	net_endpoint::operator=(std::move(endpoint));
    }

    net_endpoint& operator=(const net_endpoint&) = delete;

    constexpr net_endpoint& operator=(net_endpoint&& endpoint) noexcept
    {
	endpoint_interface::operator=(std::move(endpoint));

	::memcpy(&addr, &endpoint.addr, sizeof(addr));
	::memset(&endpoint.addr, 0, sizeof(endpoint.addr));
	
	return *this;
    }
    
    virtual ~net_endpoint() = default;

    void bind() override final
    {
	using basic_address_t = endpoint_interface::address_t;
	
	endpoint_interface::bind(reinterpret_cast<const basic_address_t*>(&addr),
				 sizeof(addr));
    }

    void connect() override final
    {
	using basic_address_t = endpoint_interface::address_t;
	
	endpoint_interface::connect(reinterpret_cast<const basic_address_t*>(&addr),
				    sizeof(addr));

	// TODO(#1): add the ability to get the socket address
    }
    
    const endpoint_interface::address_t* address() const noexcept override final
    {	
	return reinterpret_cast<const endpoint_interface::address_t*>(&addr);
    }
    
private:
    address_t addr;
};

class endpoint::tcp_endpoint final : public endpoint::net_endpoint {
public:
    using address_t = net_endpoint::address_t;

    constexpr tcp_endpoint() noexcept : net_endpoint() {}
    
    explicit constexpr tcp_endpoint(int id) noexcept : net_endpoint(id) {}

    tcp_endpoint(std::string_view sp, uint16_t port) :
	net_endpoint(::socket(AF_INET, SOCK_STREAM, 0), sp, port)
    {}
	
    tcp_endpoint(const tcp_endpoint&) = delete;

    constexpr tcp_endpoint(tcp_endpoint&& endpoint) noexcept
    {
	tcp_endpoint::operator=(std::move(endpoint));
    }

    tcp_endpoint& operator=(const tcp_endpoint&) = delete;

    constexpr tcp_endpoint& operator=(tcp_endpoint&& endpoint) noexcept
    {
	net_endpoint::operator=(std::move(endpoint));

	return *this;
    }

    ~tcp_endpoint() = default;

    void create() override
    {
	endpoint_interface::create(AF_INET, SOCK_STREAM, 0);
    }
};

#endif // ENDPOINT_HPP
