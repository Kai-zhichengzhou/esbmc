#include <goto-symex/slice.h>

symex_slicet::symex_slicet(bool assume) : ignored(0), slice_assumes(assume)
{
}

template <bool Add>
bool symex_slicet::get_symbols(const expr2tc &expr)
{
  bool res = false;
  expr->foreach_operand([this, &res](const expr2tc &e) {
    if(!is_nil_expr(e))
      res = get_symbols<Add>(e) || res;
    return res;
  });

  if(!is_symbol2t(expr))
    return res;

  const symbol2t &s = to_symbol2t(expr);
  if constexpr(Add)
    res |= depends.insert(s.get_symbol_name()).second;
  else
    res |= depends.find(s.get_symbol_name()) != depends.end();
  return res;
}

void symex_slicet::slice(std::shared_ptr<symex_target_equationt> &eq)
{
  depends.clear();

  for(symex_target_equationt::SSA_stepst::reverse_iterator it =
        eq->SSA_steps.rbegin();
      it != eq->SSA_steps.rend();
      it++)
    slice(*it);
}

void symex_slicet::slice(symex_target_equationt::SSA_stept &SSA_step)
{
  switch(SSA_step.type)
  {
  case goto_trace_stept::ASSERT:
    get_symbols<true>(SSA_step.guard);
    get_symbols<true>(SSA_step.cond);
    break;

  case goto_trace_stept::ASSUME:
    if(slice_assumes)
      slice_assume(SSA_step);
    else
    {
      get_symbols<true>(SSA_step.guard);
      get_symbols<true>(SSA_step.cond);
    }
    break;

  case goto_trace_stept::ASSIGNMENT:
    slice_assignment(SSA_step);
    break;

  case goto_trace_stept::OUTPUT:
    break;

  case goto_trace_stept::RENUMBER:
    slice_renumber(SSA_step);
    break;

  default:
    assert(false);
  }
}

void symex_slicet::slice_assume(symex_target_equationt::SSA_stept &SSA_step)
{
  if(!get_symbols<false>(SSA_step.cond))
  {
    // we don't really need it
    SSA_step.ignore = true;
    ++ignored;
  }
  else
  {
    // If we need it, add the symbols to dependency
    get_symbols<true>(SSA_step.guard);
    get_symbols<true>(SSA_step.cond);
  }
}

void symex_slicet::slice_assignment(symex_target_equationt::SSA_stept &SSA_step)
{
  assert(is_symbol2t(SSA_step.lhs));

  if(!get_symbols<false>(SSA_step.lhs))
  {
    // we don't really need it
    SSA_step.ignore = true;
    ++ignored;
  }
  else
  {
    get_symbols<true>(SSA_step.guard);
    get_symbols<true>(SSA_step.rhs);

    // Remove this symbol as we won't be seeing any references to it further
    // into the history.
    depends.erase(to_symbol2t(SSA_step.lhs).get_symbol_name());
  }
}

void symex_slicet::slice_renumber(symex_target_equationt::SSA_stept &SSA_step)
{
  assert(is_symbol2t(SSA_step.lhs));

  if(!get_symbols<false>(SSA_step.lhs))
  {
    // we don't really need it
    SSA_step.ignore = true;
    ++ignored;
  }

  // Don't collect the symbol; this insn has no effect on dependencies.
}

BigInt slice(std::shared_ptr<symex_target_equationt> &eq, bool slice_assumes)
{
  symex_slicet symex_slice(slice_assumes);
  symex_slice.slice(eq);
  return symex_slice.ignored;
}

BigInt simple_slice(std::shared_ptr<symex_target_equationt> &eq)
{
  BigInt ignored = 0;

  // just find the last assertion
  symex_target_equationt::SSA_stepst::iterator last_assertion =
    eq->SSA_steps.end();

  for(symex_target_equationt::SSA_stepst::iterator it = eq->SSA_steps.begin();
      it != eq->SSA_steps.end();
      it++)
    if(it->is_assert())
      last_assertion = it;

  // slice away anything after it

  symex_target_equationt::SSA_stepst::iterator s_it = last_assertion;

  if(s_it != eq->SSA_steps.end())
    for(s_it++; s_it != eq->SSA_steps.end(); s_it++)
    {
      s_it->ignore = true;
      ++ignored;
    }

  return ignored;
}
