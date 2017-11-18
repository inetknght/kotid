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

} // namespace koti
