#pragma once

#include "lager/detail/nodes.hpp"
#include "lager/detail/traversal.hpp"
#include <boost/intrusive/set.hpp>

namespace lager {
namespace detail {

struct node_rank_is_key
{
    using type = long;
    type operator()(const reader_node_base& x) { return x.rank(); }
};

class topo_intrusive_traversal_rb : traversal
{
    using unordered_map = boost::intrusive::multiset<
        reader_node_base,
        boost::intrusive::key_of_value<node_rank_is_key>,
        boost::intrusive::member_hook<reader_node_base,
                                      typename reader_node_base::hook_type_rb,
                                      &reader_node_base::member_hook_rb_>,
        boost::intrusive::optimize_multikey<true>>;

public:
    topo_intrusive_traversal_rb()                                   = delete;
    topo_intrusive_traversal_rb(const topo_intrusive_traversal_rb&) = delete;
    topo_intrusive_traversal_rb&
    operator=(const topo_intrusive_traversal_rb&) = delete;

    topo_intrusive_traversal_rb(reader_node_base* root, std::size_t = 0)
    {
        assert(root);
        schedule_.insert(*root);
    }
    topo_intrusive_traversal_rb(const std::shared_ptr<reader_node_base>& root,
                                std::size_t = 0)
        : topo_intrusive_traversal_rb(root.get())
    {
    }

    void visit()
    {
        while (!schedule_.empty()) {
            schedule_.erase_and_dispose(
                schedule_.begin()->rank(),
                [this](auto* n) { n->send_down(*this); });
        }
    }

    void schedule(reader_node_base* n) override final
    {
        // if node is already linked, it has already beek scheduled!
        if (!n->member_hook_rb_.is_linked()) {
            schedule_.insert(*n);
        }
    }

private:
    unordered_map schedule_{};
};

struct schedule_rank_is_key
{
    using type = long;
    const type& operator()(const node_schedule& x) { return x.rank_; }
};

struct topo_traversal_set : traversal
{
    using set = boost::intrusive::set<
        node_schedule,
        boost::intrusive::key_of_value<schedule_rank_is_key>,
        boost::intrusive::member_hook<node_schedule,
                                      typename node_schedule::hook_type_rb,
                                      &node_schedule::member_hook_rb_>>;

public:
    topo_traversal_set()                                     = delete;
    topo_traversal_set(const topo_traversal_set&)            = delete;
    topo_traversal_set& operator=(const topo_traversal_set&) = delete;

    topo_traversal_set(reader_node_base* root, std::size_t = 0)
    {
        assert(root);
        auto [it, _] = rank_schedule_.insert(root->get_node_schedule());
        it->nodes_.push_back(*root);
    }

    topo_traversal_set(const std::shared_ptr<reader_node_base>& root,
                       std::size_t = 0)
        : topo_traversal_set(root.get())
    {
    }

    void visit()
    {
        while (!rank_schedule_.empty()) {
            rank_schedule_.erase_and_dispose(
                rank_schedule_.begin(), [this](auto* r) {
                    r->nodes_.clear_and_dispose(
                        [&](auto* node) { node->send_down(*this); });
                });
        }
    }

    void schedule(reader_node_base* n) override final
    {
        // if node is already linked, it has already beek scheduled!
        if (n->member_hook_list_.is_linked())
            return;

        // we do this to support the case of visit with graphs having different
        // root nodes
        auto& it = [this, n]() -> node_schedule& {
            if (!n->get_node_schedule().member_hook_rb_.is_linked()) {
                auto [it, _] = rank_schedule_.insert(n->get_node_schedule());
                return *it;
            }
            return n->get_node_schedule();
        }();

        it.nodes_.push_back(*n);
    }

private:
    set rank_schedule_{};
};

} // namespace detail
} // namespace lager
