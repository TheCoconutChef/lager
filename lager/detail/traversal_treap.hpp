#pragma once

#include "lager/detail/nodes.hpp"
#include "traversal.hpp"
#include <boost/intrusive/treap_set.hpp>

namespace lager::detail {

class treap_traversal : public traversal
{
public:
    treap_traversal()                                  = delete;
    treap_traversal(const treap_traversal&)            = delete;
    treap_traversal& operator=(const treap_traversal&) = delete;

    treap_traversal(const std::shared_ptr<reader_node_base>& root, std::size_t)
        : treap_traversal(root.get())
    {
    }

    treap_traversal(reader_node_base* root) { hint_ = schedule_.insert(*root); }

    void visit() override
    {
        while (!schedule_.empty())
            schedule_.erase_and_dispose(
                schedule_.top(),
                [this](reader_node_base* n) { n->send_down(*this); });
    }

    void schedule(reader_node_base* n) override
    {
        if (!n->member_hook_treap_.is_linked())
            hint_ = schedule_.insert(hint_, *n);
    }

private:
    using member_option_t =
        boost::intrusive::member_hook<reader_node_base,
                                      reader_node_base::hook_type_treap,
                                      &reader_node_base::member_hook_treap_>;
    using schedule_t =
        boost::intrusive::treap_multiset<reader_node_base, member_option_t>;
    using hint_t = typename schedule_t::iterator;

    schedule_t schedule_;
    hint_t hint_;
};
} // namespace lager::detail
