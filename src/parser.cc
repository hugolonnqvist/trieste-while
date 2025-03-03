#include "lang.hh"
#include "internal.hh"

namespace whilelang
{
  using namespace trieste;
  using namespace trieste::detail;

  Parse parser()
  {
    Parse p(depth::file, parse_wf);

    auto infix = [](Make& m, Token t) {
      // This precedence table maps infix operators to the operators that have
      // *higher* precedence, and which should therefore be terminated when that
      // operator is encountered. Note that operators with the same precedence
      // terminate each other. (for reasons, it has to be defined inside the lambda)
      const auto precedence_table = std::map<Token, std::initializer_list<Token>> {
        {Mul, {}},
        {Add, {Sub, Mul}},
        {Sub, {Add, Mul}},
        {LT, {Add, Sub, Mul, Equals}},
        {Equals, {Add, Sub, Mul, LT}},
        {And, {Add, Sub, Mul, LT, Equals}},
        {Or, {Add, Sub, Mul, LT, Equals, And}},
        {Assign, {Add, Sub, Mul, Equals, LT, And, Or}},
      };

      auto skip = precedence_table.at(t);
      m.seq(t, skip);
      // Push group to be able to check that an operand follows
      m.push(Group);
    };

    auto pop_until = [](Make &m, Token t, std::initializer_list<Token> stop = {File}) {
      while (!m.in(t) && !m.group_in(t)
             && !m.in(stop)) {
        m.term();
        m.pop();
      }

      return (m.in(t) || m.group_in(t));
    };

    auto pair_with = [pop_until](Make &m, Token preceding, Token following) {
      pop_until(m, preceding, {Paren, File});
      m.term();

      if (!m.in(preceding)) {
        const std::string msg = (std::string) "Unexpected '" + following.str() + "'";
        m.error(msg);
        return;
      }

      m.pop(preceding);
      m.push(following);
    };

    p("start",
    {
        // whitespace
        "[[:space:]]+" >> [](auto&) {}, // no-op

        // Line comment.
        "//[^\n]*" >> [](auto&) {}, // no-op

        // Statements
        ":=" >> [infix](auto& m) { infix(m, Assign); },

        "skip" >> [](auto& m) { m.add(Skip); },

        ";" >> [](auto& m) { m.seq(Semi, {Add, Sub, Mul, Equals, LT, And, Or, Assign, Else, Do, Group}); },

        "if\\b" >> [](auto& m) { m.push(If); },
        "then\\b" >> [pair_with](auto& m) { pair_with(m, If, Then); },
        "else\\b" >> [pair_with](auto& m) { pair_with(m, Then, Else); },

        "while\\b" >> [](auto& m) { m.push(While); },
        "do\\b" >> [pair_with](auto& m) { pair_with(m, While, Do); },

        // Expressions
        R"(\+)" >> [infix](auto& m) { infix(m, Add); },
        "-" >> [infix](auto& m) { infix(m, Sub); },
        R"(\*)" >> [infix](auto& m) { infix(m, Mul); },

        "and\\b" >> [infix](auto& m) { infix(m, And); },
        "or\\b" >>  [infix](auto& m) { infix(m, Or); },
        "not\\b" >> [](auto& m) { m.add(Not); },

        "<" >> [infix](auto& m) { infix(m, LT); },
        "=" >> [infix](auto& m) { infix(m, Equals); },

        "true\\b" >> [](auto& m) { m.add(True); },
        "false\\b" >> [](auto& m) { m.add(False); },

        "[[:digit:]]+" >> [](auto& m) { m.add(Int); },

        // Variables
        R"([_[:alpha:]][_[:alnum:]]*)" >> [](auto& m) { m.add(Ident); },

        "\\(" >> [](auto& m) { m.push(Paren); },
        "\\)" >> [pop_until](auto& m) { pop_until(m, Paren, {Brace}); m.term(); m.pop(Paren); },

        "\\{" >> [](auto& m) { m.push(Brace); },
        "\\}" >> [pop_until](auto& m) { pop_until(m, Brace, {Paren}); m.term(); m.pop(Brace); },
    }
    );

    p.done([pop_until](auto& m) {
        pop_until(m, File, {Paren, If, Then, While});
    });
    return p;
  }
}
