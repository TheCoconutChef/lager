#pragma once

namespace lager {
namespace detail {
struct reader_node_base;

class traversal
{
public:
    virtual void schedule(reader_node_base* n) = 0;
};

} // namespace detail
} // namespace lager
