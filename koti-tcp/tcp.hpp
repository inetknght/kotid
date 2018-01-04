#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include <boost/asio.hpp>

namespace koti {

namespace asio = boost::asio;
namespace ip = asio::ip;
using tcp = ip::tcp;
using port_number = std::uint16_t;

// https://stackoverflow.com/a/32172486/1111557
class inheritable_shared_from_this
	: virtual public std::enable_shared_from_this<inheritable_shared_from_this>
{
protected:
	virtual ~inheritable_shared_from_this() = default;

	template <typename Derived>
	std::shared_ptr<Derived> shared_from()
	{
		return std::static_pointer_cast<Derived>(shared_from_this());
	}
};

} // namespace koti
