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
        // TODO: Fix ugly way of union
        auto tmp1 = get_first_basic_children(then_stmt);
        auto tmp2 = get_first_basic_children(else_stmt);

        children.insert(tmp1.begin(), tmp1.end());
        children.insert(tmp2.begin(), tmp2.end());
    } else {
        children.insert(n);
    }

    return children;
  }

  std::set<Node> get_last_basic_children(Node n) {
    std::set<Node> children;
    while (n->type() == Semi || n->type() == Stmt || n->type() == While) {
        n = n->back();
    }    

    if (n->type() == If) {
        auto then_stmt = n / Then;
        auto else_stmt = n / Else;
        // TODO: Fix ugly way of union
        auto tmp1 = get_last_basic_children(then_stmt);
        auto tmp2 = get_last_basic_children(else_stmt);

        children.insert(tmp1.begin(), tmp1.end());
        children.insert(tmp2.begin(), tmp2.end());
    } else {
        children.insert(n);
    }

    return children;
  }

  void add_predecessor(std::shared_ptr<trieste::NodeMap<trieste::NodeSet>> predecessor, Node node, Node prev) {
      while (node->type() == Semi || node->type() == Stmt) {
          node = node->front();
      }

      auto res = predecessor->find(node);
      
      if (res != predecessor->end()) {
          res->second.insert(prev);
      } else {
          predecessor->insert({node, {prev}});
      }
  } 

//   void add_predecessor(std::shared_ptr<trieste::NodeMap<trieste::NodeSet>> predecessor, Node node, Node prev) {
//       auto res = predecessor->find(node);

//       if (res != predecessor->end()) {
//           res->second.insert(prev);
//       } else {
//           predecessor->insert({node, {prev}});
//       }
//   } 

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
