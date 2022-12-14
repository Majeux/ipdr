import time
import z3

z3.set_option(dl_engine=1)
z3.set_option(dl_pdr_use_farkas=True)

def flatten(l):
    return [s for t in l for s in t]


class TransitionSystem():
    def __init__(self, initial, transitions, vars1):
        self.fp = z3.Fixedpoint()
        # == self.fp.set('engine', 'spacer') # <'bmc'|'spacer'>
        self.fp.set(engine='spacer')
        self.initial = initial
        self.transitions = transitions
        self.vars1 = vars1

    def declare_rels(self):
        B = z3.BoolSort()
        var_sorts = [v.sort() for v in self.vars1]
        state_sorts = var_sorts
        self.state_vals = [v for v in self.vars1]
        self.state_sorts = state_sorts
        self.var_sorts = var_sorts
        self.state = z3.Function('state', state_sorts + [B])
        self.step = z3.Function('step',  state_sorts + state_sorts + [B])
        self.fp.register_relation(self.state)
        self.fp.register_relation(self.step)

# Set of reachable states are transitive closure of step.

    def state0(self):
        idx = range(len(self.state_sorts))
        return self.state([z3.Var(i, self.state_sorts[i]) for i in idx])

    def state1(self):
        n = len(self.state_sorts)
        return self.state([z3.Var(i+n, self.state_sorts[i]) for i in range(n)])

    def rho(self):
        n = len(self.state_sorts)
        args1 = [z3.Var(i, self.state_sorts[i]) for i in range(n)]
        args2 = [z3.Var(i+n, self.state_sorts[i]) for i in range(n)]
        args = args1 + args2
        return self.step(args)

    def declare_reachability(self):
        self.fp.rule(self.state1(), [self.state0(), self.rho()], "reach")


# Define transition relation

    def abstract(self, e):
        n = len(self.state_sorts)
        sub = [(self.state_vals[i], z3.Var(i, self.state_sorts[i]))
               for i in range(n)]
        return z3.substitute(e, sub)

    def declare_transition(self, tr):
        len_s = len(self.state_sorts)
        effect = tr["effect"]
        vars1 = [z3.Var(i, self.state_sorts[i]) for i in range(len_s)] + effect
        rho1 = self.abstract(self.step(vars1))
        guard = self.abstract(tr["guard"])
        self.fp.rule(rho1, guard, tr["name"])

    def declare_transitions(self):
        for t in self.transitions:
            self.declare_transition(t)

    def declare_initial(self):
        self.fp.rule(self.state0(), [self.abstract(self.initial)], "I")

    def query(self, query):
        self.declare_rels()
        self.declare_initial()
        self.declare_reachability()
        self.declare_transitions()
        query = z3.And(self.state0(), self.abstract(query))
        print("Fixedpoint engine")
        print(self.fp)

        print("Query")
        print(query)

        print("Result")
        time1 = time.time()
        Q = self.fp.query(query)
        time2 = time.time()
        print(Q)
        print(f"Time passed: {round(time2-time1, 3)} sec")

        print("Answer")
        print(ptr.fp.get_rule_names_along_trace())
# print self.fp.statistics()


a_1, b_1, b_2, c_1, c_2 = z3.Bools('a_1 b_1 b_2 c_1 c_2')
true = z3.BoolVal(True)
false = z3.BoolVal(False)


t1 = {"name": "flip a_1",
      "guard": true,
      "effect": [z3.Not(a_1), b_1, b_2, c_1, c_2]}
t2 = {"name": "flip b_1",
      "guard": true,
      "effect": [a_1, z3.Not(b_1), b_2, c_1, c_2]}
t3 = {"name": "flip b_2",
      "guard": z3.And(b_1, c_2),
      "effect": [a_1, true, z3.Not(b_2), c_1, true]}
s1 = {"name": "flip c_1",
      "guard": b_1,
      "effect": [a_1, true, b_2, z3.Not(c_1), c_2]}
s2 = {"name": "flip c_2",
      "guard": z3.And(a_1, c_1),
      "effect": [true, b_1, b_2, true, z3.Not(c_2)]}

ptr = TransitionSystem(z3.And(z3.Not(a_1), z3.Not(b_1), z3.Not(b_2), z3.Not(c_1), z3.Not(c_2)),
                       [t1, t2, t3, s1, s2],
                       [a_1, b_1, b_2, c_1, c_2])

ptr.query(z3.And(a_1, z3.Not(b_1), b_2, z3.Not(c_1), c_2))
