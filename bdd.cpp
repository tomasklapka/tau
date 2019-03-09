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
#include "bdd.h"

using namespace std;

//#define MEMO
#ifdef MEMO
typedef array<size_t, 2> memo;
typedef array<size_t, 3> adtmemo;
typedef pair<const bool*, size_t> exmemo;
typedef pair<const bool*, memo> apexmemo;
typedef pair<const size_t*, size_t> permemo;
template<> struct std::hash<memo> { size_t operator()(const memo& m) const; };
template<> struct std::hash<exmemo> { size_t operator()(const exmemo&m)const;};
template<>struct std::hash<apexmemo>{size_t operator()(const apexmemo&m)const;};
template<> struct std::hash<permemo>{ size_t operator()(const permemo&m)const;};
unordered_map<memo, size_t> memo_and, memo_and_not, memo_or, memo_dt;
unordered_map<adtmemo, size_t> memo_adt;
unordered_map<exmemo, size_t> memo_ex;
unordered_map<apexmemo, size_t> memo_and_ex, memo_and_not_ex;
unordered_map<permemo, size_t> memo_permute;
#define apply_ret(r, m) return m.emplace(t, res = (r)), res
#else
#define apply_ret(r, m) return r
#endif

template<> struct std::hash<node> {
	size_t operator()(const node& n) const { return n[0] + n[1] + n[2]; }
};

vector<node> V; // all bdd nodes
unordered_map<node, size_t> M; // node to its index

size_t bdd_add_nocheck(const node& n) {
	size_t r;
	return M.emplace(n, r = V.size()), V.emplace_back(n), r;
}

const node& getnode(size_t n)	{ return V[n]; }
bool leaf(size_t x)		{ return x == T || x == F; }
bool leaf(const node& x)	{ return !x[0]; }
bool trueleaf(const node& x)	{ return leaf(x) && x[1]; }
bool trueleaf(const size_t& x)	{ return x == T; }
size_t from_bit(size_t x ,bool v) {
	return bdd_add(v ? node{{x+1,T,F}} : node{{x+1,F,T}}); }

size_t bdd_add(const node& n) {//create new bdd node,standard implementation
	if (n[1] == n[2]) return n[1];
	auto it = M.find(n);
	return it == M.end() ? bdd_add_nocheck(n) : it->second;
}

void bdd_init() { bdd_add_nocheck({{0, 0, 0}}), bdd_add_nocheck({{0, 1, 1}}); }

void sat(size_t v, size_t nvars, node n, bools& p, vbools& r) {
	if (leaf(n) && !trueleaf(n)) return;
	if (v < n[0])
		p[v-1] = true,  sat(v + 1, nvars, n, p, r),
		p[v-1] = false, sat(v + 1, nvars, n, p, r);
	else if (v != nvars+1)
		p[v-1] = true,  sat(v + 1, nvars, getnode(n[1]), p, r),
		p[v-1] = false, sat(v + 1, nvars, getnode(n[2]), p, r);
	else	r.push_back(p);
}

vbools allsat(size_t x, size_t nvars) {
	bools p(nvars);
	vbools r;
	return sat(1, nvars, getnode(x), p, r), r;
}

size_t bdd_or(size_t x, size_t y) {
	if (x == y) return x;
#ifdef MEMO
	memo t = {{x, y}};
	auto it = memo_or.find(t);
	if (it != memo_or.end()) return it->second;
	size_t res;
#endif	
	const node &Vx = getnode(x);
	if (leaf(Vx)) apply_ret(trueleaf(Vx) ? T : y, memo_or);
       	const node &Vy = getnode(y);
	if (leaf(Vy)) apply_ret(trueleaf(Vy) ? T : x, memo_or);
	const size_t &vx = Vx[0], &vy = Vy[0];
	size_t v, a = Vx[1], b = Vy[1], c = Vx[2], d = Vy[2];
	if ((!vx && vy) || (vy && (vx > vy))) a = c = x, v = vy;
	else if (!vx) apply_ret(a||b ? T : F, memo_or);
	else if ((v = vx) < vy || !vy) b = d = y;
	apply_ret(bdd_add({{v, bdd_or(a, b), bdd_or(c, d)}}), memo_or);
}

size_t bdd_ex(size_t x, const bool* b) {
	node n = getnode(x);
	if (leaf(n)) return x;
#ifdef MEMO
	exmemo t = {b, x};
	auto it = memo_ex.find(t);
	if (it != memo_ex.end()) return it->second;
	size_t res;
#endif	
	if (b[n[0]-1]) {
		if (leaf(x = bdd_or(n[1], n[2]))) apply_ret(x, memo_ex);
		n = getnode(x);
	}
	apply_ret(bdd_add({{n[0], bdd_ex(n[1], b), bdd_ex(n[2], b)}}), memo_ex);
}

size_t bdd_and(size_t x, size_t y) {
	if (x == y) return x;
#ifdef MEMO
	memo t = {{x, y}};
	auto it = memo_and.find(t);
	if (it != memo_and.end()) return it->second;
	size_t res;
#endif	
	const node &Vx = getnode(x);
	if (leaf(Vx)) apply_ret(trueleaf(Vx)?y:F, memo_and);
       	const node &Vy = getnode(y);
	if (leaf(Vy)) apply_ret(!trueleaf(Vy) ? F : x, memo_and);
	const size_t &vx = Vx[0], &vy = Vy[0];
	size_t v, a = Vx[1], b = Vy[1], c = Vx[2], d = Vy[2];
	if ((!vx && vy) || (vy && (vx > vy))) a = c = x, v = vy;
	else if (!vx) apply_ret((a&&b)?T:F, memo_and);
	else if ((v = vx) < vy || !vy) b = d = y;
	apply_ret(bdd_add({{v, bdd_and(a, b), bdd_and(c, d)}}), memo_and);
}

size_t bdd_deltail(size_t x, size_t h) {
	if (leaf(x)) return x;
#ifdef MEMO
	memo t = {{x, h}};
	auto it = memo_dt.find(t);
	if (it != memo_dt.end()) return it->second;
	size_t res;
#endif	
	node n = getnode(x);
	if (n[0] <= h)
		apply_ret(bdd_add({{n[0], bdd_deltail(n[1],h),
			bdd_deltail(n[2],h)}}), memo_dt);
	apply_ret(n[1] == F && n[2] == F ? F : T, memo_dt);
}    

size_t bdd_and_deltail(size_t x, size_t y, size_t h) {
	if (x == y) return bdd_deltail(x, h);
#ifdef MEMO
	adtmemo t = {{x, y, h}};
	auto it = memo_adt.find(t);
	if (it != memo_adt.end()) return it->second;
	size_t res;
#endif	
	const node &Vx = getnode(x);
	if (leaf(Vx)) apply_ret(trueleaf(Vx)? bdd_deltail(y, h) : F, memo_adt);
       	const node &Vy = getnode(y);
	if (leaf(Vy)) apply_ret(!trueleaf(Vy) ? F : bdd_deltail(x, h),memo_adt);
	const size_t &vx = Vx[0], &vy = Vy[0];
	size_t v, a = Vx[1], b = Vy[1], c = Vx[2], d = Vy[2];
	if ((!vx && vy) || (vy && (vx > vy))) a = c = x, v = vy;
	else if (!vx) apply_ret((a&&b)?T:F, memo_adt);
	else if ((v = vx) < vy || !vy) b = d = y;
	apply_ret(bdd_deltail(bdd_add({{v, bdd_and_deltail(a, b, h),
		bdd_and_deltail(c, d, h)}}), h), memo_adt);
}

size_t bdd_and_ex(size_t x, size_t y, const bool* s) {
	if (x == y) return bdd_ex(x, s);
#ifdef MEMO
	apexmemo t = {s, {{x, y}}};
	auto it = memo_and_ex.find(t);
	if (it != memo_and_ex.end()) return it->second;
#endif	
	size_t res;
	const node &Vx = getnode(x), &Vy = getnode(y);
	const size_t &vx = Vx[0], &vy = Vy[0];
	size_t v, a = Vx[1], b = Vy[1], c = Vx[2], d = Vy[2];
	if (leaf(Vx)) {
		res = trueleaf(Vx) ? bdd_ex(y, s) : F;
		goto ret;
	}
	if (leaf(Vy)) {
		res = trueleaf(Vy) ? bdd_ex(x, s) : F;
		goto ret;
	}
	if ((!vx && vy) || (vy && (vx > vy))) a = c = x, v = vy;
	else if (!vx) { res = (a&&b)?T:F; goto ret; }
	else if ((v = vx) < vy || !vy) b = d = y;
	if (s[v-1]) res = bdd_or(bdd_and_ex(a, b, s), bdd_and_ex(c, d, s));
	else res = bdd_add({{v, bdd_and_ex(a, b, s), bdd_and_ex(c, d, s)}});
ret:	apply_ret(res, memo_and_ex);
}

size_t bdd_and_not(size_t x, size_t y) {
	if (x == y) return F;
#ifdef MEMO
	memo t = {{x, y}};
	auto it = memo_and_not.find(t);
	if (it != memo_and_not.end()) return it->second;
	size_t res;
#endif	
	const node &Vx = getnode(x);
	if (leaf(Vx) && !trueleaf(Vx)) apply_ret(F, memo_and_not);
       	const node &Vy = getnode(y);
	if (leaf(Vy)) apply_ret(trueleaf(Vy) ? F : x, memo_and_not);
	const size_t &vx = Vx[0], &vy = Vy[0];
	size_t v, a = Vx[1], b = Vy[1], c = Vx[2], d = Vy[2];
	if ((!vx && vy) || (vy && (vx > vy))) a = c = x, v = vy;
	else if (!vx) apply_ret((a&&!b)?T:F, memo_and_not);
	else if ((v = vx) < vy || !vy) b = d = y;
	apply_ret(bdd_add({{v,bdd_and_not(a,b),bdd_and_not(c,d)}}),memo_and_not);
}

size_t bdd_and_not_ex(size_t x, size_t y, const bool* s) {
	if (x == y) return F;
#ifdef MEMO
	apexmemo t = {s, {{x, y}}};
	auto it = memo_and_not_ex.find(t);
	if (it != memo_and_not_ex.end()) return it->second;
#endif	
	size_t res;
	const node &Vx = getnode(x), &Vy = getnode(y);
	const size_t &vx = Vx[0], &vy = Vy[0];
	size_t v, a = Vx[1], b = Vy[1], c = Vx[2], d = Vy[2];
	/*if (leaf(Vx) && !trueleaf(Vx)) {
		res = F;
		goto ret;
	}
	if (leaf(Vy)) {
		res = trueleaf(Vy) ? F : bdd_ex(x, s);
		goto ret;
	}*/
	if ((!vx && vy) || (vy && (vx > vy))) a = c = x, v = vy;
	else if (!vx) { res = (a && !b)?T:F; goto ret; }
	else if ((v = vx) < vy || !vy) b = d = y;
	if (s[v-1]) res = bdd_or(bdd_and_not_ex(a,b,s), bdd_and_not_ex(c,d,s));
	else res = bdd_add({{v, bdd_and_not_ex(a,b,s), bdd_and_not_ex(c,d,s)}});
ret:	apply_ret(res, memo_and_not_ex);
}

size_t bdd_ite(size_t v, size_t t, size_t e) {
	const node &x = getnode(t), &y = getnode(e);
	if ((leaf(x)||v<x[0]) && (leaf(y)||v<y[0])) return bdd_add({{v+1,t,e}});
	return bdd_or(bdd_and(from_bit(v,true),t),bdd_and(from_bit(v,false),e));
}

size_t bdd_permute(size_t x, const size_t* m) {//overlapping rename
#ifdef MEMO
	permemo t = {m, x};
	auto it = memo_permute.find(t);
	if (it != memo_permute.end()) return it->second;
	size_t res;
#endif	
	if (leaf(x)) return x;
	const node n = getnode(x);
	apply_ret(bdd_ite(m[n[0]-1], bdd_permute(n[1], m), bdd_permute(n[2],m)),
		memo_permute);
}

size_t from_eq(size_t x, size_t y) {
	return bdd_or(	bdd_and(from_bit(x, true), from_bit(y, true)),
			bdd_and(from_bit(x, false),from_bit(y, false)));
}

void memos_clear() {
#ifdef MEMO		
	memo_and.clear(), memo_and_not.clear(), memo_or.clear(),
	memo_permute.clear(), memo_and_ex.clear(), memo_and_not_ex.clear();
#endif		
}
