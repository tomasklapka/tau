// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.
#include <map>
#include <vector>
#include "bdd.h"

typedef int_t ntable;
typedef int_t rel_t;
struct raw_term;
struct raw_prog;
struct raw_rule;
class tables;
class dict_t;

typedef std::pair<rel_t, ints> sig;

class tables {
	typedef std::map<int_t, size_t> varmap;
	typedef std::function<void(const raw_term&)> rt_printer;
	struct term : public ints {
		bool neg;
		ntable tab;
		term() {}
		term(bool neg, ntable tab, const ints& args) :
			ints(args), neg(neg), tab(tab) {}
		bool operator<(const term& t) const {
			if (neg != t.neg) return neg;
			if (tab != t.tab) return tab < t.tab;
			return (const ints&)*this < t;
		}
		void replace(const std::map<int_t, int_t>& m);
	};
	struct body {
		bool neg, ext = false;
		ntable tab;
		bools ex;
		uints perm;
		spbdd_handle q, tlast, rlast;
		struct pbodycmp {
			bool operator()(const body* x, const body* y) const {
				return *x < *y;
			}
		};
		static std::set<body*, pbodycmp> s;
		bool operator<(const body& t) const {
			if (q != t.q) return q < t.q;
			if (neg != t.neg) return neg;
			if (ext != t.ext) return ext;
			if (tab != t.tab) return tab < t.tab;
			if (ex != t.ex) return ex < t.ex;
			return perm < t.perm;
		}
	};
	struct alt : public std::vector<body*> {
		spbdd_handle rng, rlast = bdd_handle::F;
		size_t varslen;
		bdd_handles last;
		std::vector<term> t;
		std::vector<std::pair<uints, bools>> order;
		bools ex;
		uints perm;
		bool operator<(const alt& t) const {
			if (varslen != t.varslen) return varslen < t.varslen;
			if (rng != t.rng) return rng < t.rng;
			return (std::vector<body*>)*this<(std::vector<body*>)t;
		}
	};
	struct rule : public std::vector<alt> {
		bool neg;
		ntable tab;
		spbdd_handle eq, rlast = bdd_handle::F;
		size_t len;
		bdd_handles last;
		term t;
		bool operator<(const rule& t) const {
			if (neg != t.neg) return neg;
			if (tab != t.tab) return tab < t.tab;
			if (eq != t.eq) return eq < t.eq;
			return (std::vector<alt>)*this < (std::vector<alt>)t;
		}
	};
	struct table {
		sig s;
		size_t len;
		spbdd_handle t = bdd_handle::F;
		bdd_handles add, del;
		bool ext = true; // extensional
		bool commit(DBG(size_t));
	};
	std::vector<table> ts;
	std::map<sig, ntable> smap;
	std::vector<rule> rules;
	alt get_alt(const std::vector<raw_term>&);
	rule get_rule(const raw_rule&);
	void get_sym(int_t s, size_t arg, size_t args, spbdd_handle& r) const;
	void get_var_ex(size_t arg, size_t args, bools& b) const;
	void get_alt_ex(alt& a, const term& h) const;
	void merge_extensionals();

	int_t syms = 0, nums = 0, chars = 0;
	size_t bits = 2;
	dict_t& dict;
	bool datalog;

	size_t max_args = 0;
	std::map<std::array<int_t, 6>, spbdd_handle> range_memo;

	size_t pos(size_t bit, size_t nbits, size_t arg, size_t args) const {
		DBG(assert(bit < nbits && arg < args);)
		return (nbits - bit - 1) * args + arg;
	}

	size_t pos(size_t bit_from_right, size_t arg, size_t args) const {
		DBG(assert(bit_from_right < bits && arg < args);)
		return (bits - bit_from_right - 1) * args + arg;
	}

	size_t arg(size_t v, size_t args) const {
		return v % args;
	}

	size_t bit(size_t v, size_t args) const {
		return bits - 1 - v / args;
	}

	spbdd_handle from_bit(size_t b, size_t arg, size_t args, int_t n) const{
		return ::from_bit(pos(b, arg, args), n & (1 << b));
	}

	void add_bit();
	spbdd_handle add_bit(spbdd_handle x, size_t args);
	spbdd_handle leq_const(int_t c, size_t arg, size_t args, size_t bit)
		const;
	void range(size_t arg, size_t args, bdd_handles& v);
	spbdd_handle range(size_t arg, ntable tab);
	void range_clear_memo() { range_memo.clear(); }

	sig get_sig(const raw_term& t);
	ntable add_table(sig s);
	uints get_perm(const term& t, const varmap& m, size_t len) const;
	template<typename T>
	static varmap get_varmap(const term& h, const T& b, size_t &len);
	spbdd_handle get_alt_range(const term& h, const std::set<term>& a,
			const varmap& vm, size_t len);
	spbdd_handle from_term(const term&, body *b = 0,
		std::map<int_t, size_t>*m = 0, size_t hvars = 0);
	body get_body(const term& t, const varmap&, size_t len) const;
	void align_vars(term& h, std::set<term>& b) const;
	spbdd_handle from_fact(const term& t);
	term from_raw_term(const raw_term&);
	std::pair<bools, uints> deltail(size_t len1, size_t len2) const;
	spbdd_handle body_query(body& b, size_t);
	void alt_query(alt& a, bdd_handles& v, size_t);
	DBG(vbools allsat(spbdd_handle x, size_t args) const;)
	void out(std::wostream&, spbdd_handle, ntable) const;
	void out(spbdd_handle, ntable, const rt_printer&) const;
	void get_rules(const raw_prog& p);
	void add_prog(const raw_prog& p);
	bool fwd();
	std::set<std::pair<ntable, spbdd_handle>> get_front() const;
public:
	tables();
	~tables();
	bool run_prog(const raw_prog& p) { add_prog(p); return pfp(); }
	bool pfp();
	void out(std::wostream&) const;
	void out(const rt_printer&) const;
};

std::wostream& operator<<(std::wostream& os, const vbools& x);
