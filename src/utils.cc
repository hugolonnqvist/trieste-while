#include "lang.hh"

namespace whilelang {
  using namespace trieste;

  Node get_first_basic_child(Node n) {
    while (n->type() == Semi || n->type() == Stmt || n->type() == While || n->type() == If) {
        n = n->front();
    }    
    return n;
  }

  std::set<Node> get_first_basic_children(Node n) {
    std::set<Node> children;
    while (n->type() == Semi || n->type() == Stmt || n->type() == While) {
        n = n->front();
    }    
    if (n->type() == If) {
        auto then_stmt = n / Then;
        auto else_stmt = n / Else;
        children.insert(get_first_basic_child(then_stmt));
        children.insert(get_first_basic_child(else_stmt));
    } else {
        children.insert(n);
    }

    return children;
  }

  Node get_last_basic_child(Node n) {
    while (n->type() == Semi || n->type() == Stmt) {
        n = n->back();
    }    
    return n;
  }

  void add_predecessor(std::shared_ptr<trieste::NodeMap<trieste::NodeSet>> predecessor, Node node, Node prev) {
      auto res = predecessor->find(node);

      if (res != predecessor->end()) {
          res->second.insert(prev);
      } else {
          predecessor->insert({node, {prev}});
      }
  } 

  void add_predecessor(std::shared_ptr<trieste::NodeMap<trieste::NodeSet>> predecessor, Node node, std::set<Node> prevs) {
      auto res = predecessor->find(node);

      if (res != predecessor->end()) {
          for (auto prev : prevs) {
            res->second.insert(prev);
          }
      } else {
          predecessor->insert({node, prevs});
      }
  } 
}
