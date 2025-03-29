#pragma once
#include "lang.hh"

namespace whilelang
{
  using namespace ::trieste;

  Node get_first_basic_child(Node n);

  std::set<Node> get_first_basic_children(Node n);

  Node get_last_basic_child(Node n);

  void add_predecessor(std::shared_ptr<trieste::NodeMap<trieste::NodeSet>> predecessor, Node node, Node prev);

  void add_predecessor(std::shared_ptr<trieste::NodeMap<trieste::NodeSet>> predecessor, Node node, std::set<Node> prevs);
}