template <typename Vec> 
void Frame::sat_cube(Vec& v)
{
	model_used = true;
	z3::model m = consecution_solver.get_model();
	for (unsigned i = 0; i < m.num_consts(); i++)
		v.push_back(m.get_const_interp(m.get_const_decl(i)));
}

template <typename Vec, typename UnaryPredicate> 
void Frame::sat_cube(Vec& v, UnaryPredicate p)
{
	model_used = true;
	z3::model m = consecution_solver.get_model();
	for (unsigned i = 0; i < m.size(); i++)
	{
		z3::func_decl f = m[i];
		expr b_value = m.get_const_interp(f);
		expr literal(*ctx);
		if (b_value.is_true())
			 literal = f();
		else if (b_value.is_false())
			 literal = !f();
		else throw std::runtime_error("model contains non-constant");
		
		if (p(f()) == true) 
			v.push_back(literal);
	}
}

template <typename UnaryPredicate, typename Transform> 
expr_vector Frame::unsat_core(UnaryPredicate p, Transform t) const 
{ 
	expr_vector full_core = consecution_solver.unsat_core(); 
	if (full_core.size() == 0)
		return full_core;

	expr_vector core(full_core[0].ctx());
		for (const expr& e : full_core)
			if (p(e))
				core.push_back(t(e));
		return core;
}
	
template <typename Vec, typename UnaryPredicate, typename VecReserve>
void Frame::sat_cube(Vec& v, UnaryPredicate p, VecReserve reserve)
{
	model_used = true;
	z3::model m = consecution_solver.get_model();
	reserve(m.num_consts());
	for (unsigned i = 0; i < m.num_consts(); i++)
	{
		expr e = m.get_const_interp(m.get_const_decl(i));
		if (p(e) == true) 
			v.push_back(e);
	}
}
