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

private:
    int identifier;
};

class endpoint::net_endpoint : public endpoint::endpoint_interface {
public:
    using address_t = struct ::sockaddr_in;

    constexpr net_endpoint() noexcept : endpoint_interface() {}

    net_endpoint(const net_endpoint&) = delete;

    constexpr net_endpoint(net_endpoint&& endpoint) noexcept
    {
	endpoint_interface::operator=(std::move(endpoint));
    }

    net_endpoint& operator=(const net_endpoint&) = delete;

    constexpr net_endpoint& operator=(net_endpoint&& endpoint) noexcept
    {
	endpoint_interface::operator=(std::move(endpoint));
	
	return *this;
    }
    
    virtual ~net_endpoint() = default;
    
    virtual void create() = 0;

    address_t bind(std::string_view sp, uint16_t port) const
    {
	address_t taddr = init_addr(sp, port);

	using basic_addr_t = endpoint_interface::address_t;

	endpoint_interface::bind(reinterpret_cast<const basic_addr_t*>(&taddr),
				 sizeof(taddr));

	return taddr;
    }

    address_t connect(std::string_view sp, uint16_t port) const
    {
	address_t taddr = init_addr(sp, port);

	using basic_addr_t = endpoint_interface::address_t;

	endpoint_interface::connect(reinterpret_cast<const basic_addr_t*>(&taddr),
				    sizeof(taddr));

	return taddr;
    }

private:
    address_t init_addr(std::string_view sp, uint16_t port) const
    {
	address_t taddr;
	
	::memset(&taddr, 0, sizeof(taddr));
	
	taddr.sin_family = AF_INET;
	taddr.sin_port   = ::htons(port);
	
	if (::inet_aton(sp.data(), &taddr.sin_addr) == 0)
	    throw std::runtime_error {"IPv4 address is invalid"};
	
	return taddr;
    }
};

class endpoint::tcp_endpoint final : public endpoint::net_endpoint {
public:
    using address_t = net_endpoint::address_t;
    
    constexpr tcp_endpoint() noexcept : net_endpoint() {}
    
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
