#ifndef CLIENT_SERVER_HPP
#define CLIENT_SERVER_HPP

#include <type_traits>
#include <cassert>
#include <memory>

#include "endpoint.hpp"

namespace endpoint {
    class client;
    class server;

    inline constexpr bool memempty(const void* pointer, std::size_t n) noexcept
    {
	auto p = reinterpret_cast<const char*>(pointer);

	for (; n; --n) {
	    if (*p != 0)
		return false;

	    ++p;
	}

	return true;
    }

    inline constexpr bool memnempty(const void* pointer, std::size_t n) noexcept
    {
	return (!memempty(pointer, n));
    }
}

class endpoint::client {
public:
    using address_t = endpoint_interface::address_t;
    
    static client& instance()
    {
	static client instance_;

	return instance_;
    }

    template<typename T>
    void instantation(T&& endpoint) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
	active_endpoint = std::make_unique<T>(std::move(endpoint));
    }

    void connection() const
    {
	assert(active_endpoint);
	
	active_endpoint->connect();
    }

    template<typename T, typename... Args>
    void connection(Args&&... args) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
	assert(active_endpoint);
	
	if (endpoint::memnempty(active_endpoint->address(), sizeof(typename T::address_t))) {
	    active_endpoint = std::make_unique<T>(active_endpoint->liberation(),
						  std::forward<Args>(args)...);
	} else {
	    active_endpoint = std::make_unique<T>(std::forward<Args>(args)...);
	    
	}
	
	client::connection();
    }
    
    const address_t* address() const noexcept
    {
	assert(active_endpoint);
	
	return active_endpoint->address();
    }

    virtual ~client() = default;
    
protected:
    constexpr client() noexcept : active_endpoint {nullptr} {}

private:
    std::unique_ptr<endpoint_interface> active_endpoint;    
};

class endpoint::server final : public endpoint::client {
public:
    static server& instance()
    {
	static server instance_;
	
	return instance_;
    }

    template<typename T>
    void instantation(T&& endpoint) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
	passive_endpoint = std::make_unique<T>(std::move(endpoint));
    }

    void bind() const
    {
	assert(passive_endpoint);
	
	passive_endpoint->bind();
	passive_endpoint->listen(server::queue);
    }

    template<typename T, typename... Args>
    void bind(Args&&... args) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
	assert(passive_endpoint);
	
	if (endpoint::memnempty(passive_endpoint->address(), sizeof(typename T::address_t))) {
	    passive_endpoint = std::make_unique<T>(passive_endpoint->liberation(),
						   std::forward<Args>(args)...);
	} else {
	    passive_endpoint = std::make_unique<T>(std::forward<Args>(args)...);
	    
	}
	
	server::bind();
    }

    static std::size_t queue;
    
private:
    constexpr server() noexcept : client() {}

    std::unique_ptr<endpoint_interface> passive_endpoint;
};

std::size_t endpoint::server::queue = SOMAXCONN;

#endif // CLIENT_SERVER_HPP
