var e = 0,
	t = 1;

function n(e) {
	let t = e.length;
	for (; --t >= 0;) e[t] = 0
}
var r = 1,
	i = 29,
	a = 256,
	o = 286,
	s = 30,
	c = 19,
	l = 573,
	u = 15,
	d = 16,
	f = 256,
	p = 16,
	m = 17,
	h = 18,
	g = new Uint8Array([0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0]),
	_ = new Uint8Array([0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13]),
	v = new Uint8Array([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 7]),
	y = new Uint8Array([16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15]),
	b = 512,
	x = Array(288 * 2);
n(x);
var S = Array(s * 2);
n(S);
var C = Array(b);
n(C);
var w = Array(256);
n(w);
var T = Array(i);
n(T);
var E = Array(s);
n(E);
var D = class {
		constructor(e, t, n, r, i) {
			this.static_tree = e, this.extra_bits = t, this.extra_base = n, this.elems = r, this.max_length = i, this.has_stree = e && e.length
		}
	},
	O, ee, te, k = class {
		constructor(e, t) {
			this.dyn_tree = e, this.max_code = 0, this.stat_desc = t
		}
	},
	ne = e => e < 256 ? C[e] : C[256 + (e >>> 7)],
	re = (e, t) => {
		e.pending_buf[e.pending++] = t & 255, e.pending_buf[e.pending++] = t >>> 8 & 255
	},
	A = (e, t, n) => {
		e.bi_valid > d - n ? (e.bi_buf |= t << e.bi_valid & 65535, re(e, e.bi_buf), e.bi_buf = t >> d - e.bi_valid, e.bi_valid += n - d) : (e.bi_buf |= t << e.bi_valid & 65535, e.bi_valid += n)
	},
	j = (e, t, n) => {
		A(e, n[t * 2], n[t * 2 + 1])
	},
	ie = (e, t) => {
		let n = 0;
		do n |= e & 1, e >>>= 1, n <<= 1; while (--t > 0);
		return n >>> 1
	},
	ae = e => {
		e.bi_valid === 16 ? (re(e, e.bi_buf), e.bi_buf = 0, e.bi_valid = 0) : e.bi_valid >= 8 && (e.pending_buf[e.pending++] = e.bi_buf & 255, e.bi_buf >>= 8, e.bi_valid -= 8)
	},
	oe = (e, t) => {
		let n = t.dyn_tree,
			r = t.max_code,
			i = t.stat_desc.static_tree,
			a = t.stat_desc.has_stree,
			o = t.stat_desc.extra_bits,
			s = t.stat_desc.extra_base,
			c = t.stat_desc.max_length,
			d, f, p, m, h, g, _ = 0;
		for (m = 0; m <= u; m++) e.bl_count[m] = 0;
		for (n[e.heap[e.heap_max] * 2 + 1] = 0, d = e.heap_max + 1; d < l; d++) f = e.heap[d], m = n[n[f * 2 + 1] * 2 + 1] + 1, m > c && (m = c, _++), n[f * 2 + 1] = m, !(f > r) && (e.bl_count[m]++, h = 0, f >= s && (h = o[f - s]), g = n[f * 2], e.opt_len += g * (m + h), a && (e.static_len += g * (i[f * 2 + 1] + h)));
		if (_ !== 0) {
			do {
				for (m = c - 1; e.bl_count[m] === 0;) m--;
				e.bl_count[m]--, e.bl_count[m + 1] += 2, e.bl_count[c]--, _ -= 2
			} while (_ > 0);
			for (m = c; m !== 0; m--)
				for (f = e.bl_count[m]; f !== 0;) p = e.heap[--d], !(p > r) && (n[p * 2 + 1] !== m && (e.opt_len += (m - n[p * 2 + 1]) * n[p * 2], n[p * 2 + 1] = m), f--)
		}
	},
	se = (e, t, n) => {
		let r = Array(16),
			i = 0,
			a, o;
		for (a = 1; a <= u; a++) i = i + n[a - 1] << 1, r[a] = i;
		for (o = 0; o <= t; o++) {
			let t = e[o * 2 + 1];
			t !== 0 && (e[o * 2] = ie(r[t]++, t))
		}
	},
	ce = () => {
		let e, t, n, r, a, l = Array(16);
		for (n = 0, r = 0; r < i - 1; r++)
			for (T[r] = n, e = 0; e < 1 << g[r]; e++) w[n++] = r;
		for (w[n - 1] = r, a = 0, r = 0; r < 16; r++)
			for (E[r] = a, e = 0; e < 1 << _[r]; e++) C[a++] = r;
		for (a >>= 7; r < s; r++)
			for (E[r] = a << 7, e = 0; e < 1 << _[r] - 7; e++) C[256 + a++] = r;
		for (t = 0; t <= u; t++) l[t] = 0;
		for (e = 0; e <= 143;) x[e * 2 + 1] = 8, e++, l[8]++;
		for (; e <= 255;) x[e * 2 + 1] = 9, e++, l[9]++;
		for (; e <= 279;) x[e * 2 + 1] = 7, e++, l[7]++;
		for (; e <= 287;) x[e * 2 + 1] = 8, e++, l[8]++;
		for (se(x, 287, l), e = 0; e < s; e++) S[e * 2 + 1] = 5, S[e * 2] = ie(e, 5);
		O = new D(x, g, 257, o, u), ee = new D(S, _, 0, s, u), te = new D([], v, 0, c, 7)
	},
	le = e => {
		let t;
		for (t = 0; t < o; t++) e.dyn_ltree[t * 2] = 0;
		for (t = 0; t < s; t++) e.dyn_dtree[t * 2] = 0;
		for (t = 0; t < c; t++) e.bl_tree[t * 2] = 0;
		e.dyn_ltree[f * 2] = 1, e.opt_len = e.static_len = 0, e.sym_next = e.matches = 0
	},
	ue = e => {
		e.bi_valid > 8 ? re(e, e.bi_buf) : e.bi_valid > 0 && (e.pending_buf[e.pending++] = e.bi_buf), e.bi_buf = 0, e.bi_valid = 0
	},
	de = (e, t, n, r) => {
		let i = t * 2,
			a = n * 2;
		return e[i] < e[a] || e[i] === e[a] && r[t] <= r[n]
	},
	fe = (e, t, n) => {
		let r = e.heap[n],
			i = n << 1;
		for (; i <= e.heap_len && (i < e.heap_len && de(t, e.heap[i + 1], e.heap[i], e.depth) && i++, !de(t, r, e.heap[i], e.depth));) e.heap[n] = e.heap[i], n = i, i <<= 1;
		e.heap[n] = r
	},
	pe = (e, t, n) => {
		let r, i, o = 0,
			s, c;
		if (e.sym_next !== 0)
			do r = e.pending_buf[e.sym_buf + o++] & 255, r += (e.pending_buf[e.sym_buf + o++] & 255) << 8, i = e.pending_buf[e.sym_buf + o++], r === 0 ? j(e, i, t) : (s = w[i], j(e, s + a + 1, t), c = g[s], c !== 0 && (i -= T[s], A(e, i, c)), r--, s = ne(r), j(e, s, n), c = _[s], c !== 0 && (r -= E[s], A(e, r, c))); while (o < e.sym_next);
		j(e, f, t)
	},
	me = (e, t) => {
		let n = t.dyn_tree,
			r = t.stat_desc.static_tree,
			i = t.stat_desc.has_stree,
			a = t.stat_desc.elems,
			o, s, c = -1,
			u;
		for (e.heap_len = 0, e.heap_max = l, o = 0; o < a; o++) n[o * 2] === 0 ? n[o * 2 + 1] = 0 : (e.heap[++e.heap_len] = c = o, e.depth[o] = 0);
		for (; e.heap_len < 2;) u = e.heap[++e.heap_len] = c < 2 ? ++c : 0, n[u * 2] = 1, e.depth[u] = 0, e.opt_len--, i && (e.static_len -= r[u * 2 + 1]);
		for (t.max_code = c, o = e.heap_len >> 1; o >= 1; o--) fe(e, n, o);
		u = a;
		do o = e.heap[1], e.heap[1] = e.heap[e.heap_len--], fe(e, n, 1), s = e.heap[1], e.heap[--e.heap_max] = o, e.heap[--e.heap_max] = s, n[u * 2] = n[o * 2] + n[s * 2], e.depth[u] = (e.depth[o] >= e.depth[s] ? e.depth[o] : e.depth[s]) + 1, n[o * 2 + 1] = n[s * 2 + 1] = u, e.heap[1] = u++, fe(e, n, 1); while (e.heap_len >= 2);
		e.heap[--e.heap_max] = e.heap[1], oe(e, t), se(n, c, e.bl_count)
	},
	he = (e, t, n) => {
		let r, i = -1,
			a, o = t[1],
			s = 0,
			c = 7,
			l = 4;
		for (o === 0 && (c = 138, l = 3), t[(n + 1) * 2 + 1] = 65535, r = 0; r <= n; r++) a = o, o = t[(r + 1) * 2 + 1], !(++s < c && a === o) && (s < l ? e.bl_tree[a * 2] += s : a === 0 ? s <= 10 ? e.bl_tree[m * 2]++ : e.bl_tree[h * 2]++ : (a !== i && e.bl_tree[a * 2]++, e.bl_tree[p * 2]++), s = 0, i = a, o === 0 ? (c = 138, l = 3) : a === o ? (c = 6, l = 3) : (c = 7, l = 4))
	},
	ge = (e, t, n) => {
		let r, i = -1,
			a, o = t[1],
			s = 0,
			c = 7,
			l = 4;
		for (o === 0 && (c = 138, l = 3), r = 0; r <= n; r++)
			if (a = o, o = t[(r + 1) * 2 + 1], !(++s < c && a === o)) {
				if (s < l)
					do j(e, a, e.bl_tree); while (--s !== 0);
				else a === 0 ? s <= 10 ? (j(e, m, e.bl_tree), A(e, s - 3, 3)) : (j(e, h, e.bl_tree), A(e, s - 11, 7)) : (a !== i && (j(e, a, e.bl_tree), s--), j(e, p, e.bl_tree), A(e, s - 3, 2));
				s = 0, i = a, o === 0 ? (c = 138, l = 3) : a === o ? (c = 6, l = 3) : (c = 7, l = 4)
			}
	},
	_e = e => {
		let t;
		for (he(e, e.dyn_ltree, e.l_desc.max_code), he(e, e.dyn_dtree, e.d_desc.max_code), me(e, e.bl_desc), t = c - 1; t >= 3 && e.bl_tree[y[t] * 2 + 1] === 0; t--);
		return e.opt_len += 3 * (t + 1) + 5 + 5 + 4, t
	},
	ve = (e, t, n, r) => {
		let i;
		for (A(e, t - 257, 5), A(e, n - 1, 5), A(e, r - 4, 4), i = 0; i < r; i++) A(e, e.bl_tree[y[i] * 2 + 1], 3);
		ge(e, e.dyn_ltree, t - 1), ge(e, e.dyn_dtree, n - 1)
	},
	ye = n => {
		let r = 4093624447,
			i;
		for (i = 0; i <= 31; i++, r >>>= 1)
			if (r & 1 && n.dyn_ltree[i * 2] !== 0) return e;
		if (n.dyn_ltree[18] !== 0 || n.dyn_ltree[20] !== 0 || n.dyn_ltree[26] !== 0) return t;
		for (i = 32; i < a; i++)
			if (n.dyn_ltree[i * 2] !== 0) return t;
		return e
	},
	be = !1,
	xe = e => {
		be || (ce(), be = !0), e.l_desc = new k(e.dyn_ltree, O), e.d_desc = new k(e.dyn_dtree, ee), e.bl_desc = new k(e.bl_tree, te), e.bi_buf = 0, e.bi_valid = 0, le(e)
	},
	Se = (e, t, n, r) => {
		A(e, 0 + +!!r, 3), ue(e), re(e, n), re(e, ~n), n && e.pending_buf.set(e.window.subarray(t, t + n), e.pending), e.pending += n
	},
	Ce = e => {
		A(e, r << 1, 3), j(e, f, x), ae(e)
	},
	we = (e, t, n, i) => {
		let a, o, s = 0;
		e.level > 0 ? (e.strm.data_type === 2 && (e.strm.data_type = ye(e)), me(e, e.l_desc), me(e, e.d_desc), s = _e(e), a = e.opt_len + 3 + 7 >>> 3, o = e.static_len + 3 + 7 >>> 3, o <= a && (a = o)) : a = o = n + 5, n + 4 <= a && t !== -1 ? Se(e, t, n, i) : e.strategy === 4 || o === a ? (A(e, (r << 1) + +!!i, 3), pe(e, x, S)) : (A(e, 4 + +!!i, 3), ve(e, e.l_desc.max_code + 1, e.d_desc.max_code + 1, s + 1), pe(e, e.dyn_ltree, e.dyn_dtree)), le(e), i && ue(e)
	},
	M = (e, t, n) => (e.pending_buf[e.sym_buf + e.sym_next++] = t, e.pending_buf[e.sym_buf + e.sym_next++] = t >> 8, e.pending_buf[e.sym_buf + e.sym_next++] = n, t === 0 ? e.dyn_ltree[n * 2]++ : (e.matches++, t--, e.dyn_ltree[(w[n] + a + 1) * 2]++, e.dyn_dtree[ne(t) * 2]++), e.sym_next === e.sym_end),
	Te = (e, t, n, r) => {
		let i = e & 65535 | 0,
			a = e >>> 16 & 65535 | 0,
			o = 0;
		for (; n !== 0;) {
			o = n > 2e3 ? 2e3 : n, n -= o;
			do i = i + t[r++] | 0, a = a + i | 0; while (--o);
			i %= 65521, a %= 65521
		}
		return i | a << 16 | 0
	},
	Ee = new Uint32Array((() => {
		let e, t = [];
		for (var n = 0; n < 256; n++) {
			e = n;
			for (var r = 0; r < 8; r++) e = e & 1 ? 3988292384 ^ e >>> 1 : e >>> 1;
			t[n] = e
		}
		return t
	})()),
	N = (e, t, n, r) => {
		let i = Ee,
			a = r + n;
		e ^= -1;
		for (let n = r; n < a; n++) e = e >>> 8 ^ i[(e ^ t[n]) & 255];
		return e ^ -1
	},
	P = {
		2: `need dictionary`,
		1: `stream end`,
		0: ``,
		"-1": `file error`,
		"-2": `stream error`,
		"-3": `data error`,
		"-4": `insufficient memory`,
		"-5": `buffer error`,
		"-6": `incompatible version`
	},
	De = 0,
	Oe = 1,
	ke = 2,
	Ae = 3,
	je = 4,
	Me = 5,
	Ne = 6,
	Pe = 0,
	Fe = 1,
	Ie = 2,
	Le = -1,
	Re = -2,
	ze = -3,
	Be = -4,
	Ve = -5,
	He = 9,
	Ue = 15,
	We = 8,
	Ge = 573,
	F = 3,
	I = 258,
	L = 262,
	Ke = 32,
	R = 42,
	qe = 57,
	Je = 69,
	Ye = 73,
	Xe = 91,
	Ze = 103,
	z = 113,
	Qe = 666,
	B = 1,
	V = 2,
	H = 3,
	$e = 4,
	et = 3,
	U = (e, t) => (e.msg = P[t], t),
	tt = e => e * 2 - (e > 4 ? 9 : 0),
	W = e => {
		let t = e.length;
		for (; --t >= 0;) e[t] = 0
	},
	nt = e => {
		let t, n, r, i = e.w_size;
		t = e.hash_size, r = t;
		do n = e.head[--r], e.head[r] = n >= i ? n - i : 0; while (--t);
		t = i, r = t;
		do n = e.prev[--r], e.prev[r] = n >= i ? n - i : 0; while (--t)
	},
	rt = (e, t, n) => (t << e.hash_shift ^ n) & e.hash_mask,
	G = (e, t) => {
		let n;
		if (e.legacy_hash) n = e.ins_h = rt(e, e.ins_h, e.window[t + F - 1]);
		else {
			let r = e.window,
				i = r[t] | r[t + 1] << 8 | r[t + 2] << 16 | r[t + 3] << 24;
			n = e.ins_h = Math.imul(i, 66521) + 66521 >>> 16 & e.hash_mask
		}
		let r = e.prev[t & e.w_mask] = e.head[n];
		return e.head[n] = t, r
	},
	K = e => {
		let t = e.state,
			n = t.pending;
		n > e.avail_out && (n = e.avail_out), n !== 0 && (e.output.set(t.pending_buf.subarray(t.pending_out, t.pending_out + n), e.next_out), e.next_out += n, t.pending_out += n, e.total_out += n, e.avail_out -= n, t.pending -= n, t.pending === 0 && (t.pending_out = 0))
	},
	q = (e, t) => {
		we(e, e.block_start >= 0 ? e.block_start : -1, e.strstart - e.block_start, t), e.block_start = e.strstart, K(e.strm)
	},
	J = (e, t) => {
		e.pending_buf[e.pending++] = t
	},
	it = (e, t) => {
		e.pending_buf[e.pending++] = t >>> 8 & 255, e.pending_buf[e.pending++] = t & 255
	},
	at = (e, t, n, r) => {
		let i = e.avail_in;
		return i > r && (i = r), i === 0 ? 0 : (e.avail_in -= i, t.set(e.input.subarray(e.next_in, e.next_in + i), n), e.state.wrap === 1 ? e.adler = Te(e.adler, t, i, n) : e.state.wrap === 2 && (e.adler = N(e.adler, t, i, n)), e.next_in += i, e.total_in += i, i)
	},
	ot = (e, t) => {
		let n = e.max_chain_length,
			r = e.strstart,
			i, a, o = e.prev_length,
			s = e.nice_match,
			c = e.strstart > e.w_size - L ? e.strstart - (e.w_size - L) : 0,
			l = e.window,
			u = e.w_mask,
			d = e.prev,
			f = e.strstart + I,
			p = l[r + o - 1],
			m = l[r + o];
		e.prev_length >= e.good_match && (n >>= 2), s > e.lookahead && (s = e.lookahead);
		do {
			if (i = t, l[i + o] !== m || l[i + o - 1] !== p || l[i] !== l[r] || l[++i] !== l[r + 1]) continue;
			r += 2, i++;
			do; while (l[++r] === l[++i] && l[++r] === l[++i] && l[++r] === l[++i] && l[++r] === l[++i] && l[++r] === l[++i] && l[++r] === l[++i] && l[++r] === l[++i] && l[++r] === l[++i] && r < f);
			if (a = I - (f - r), r = f - I, a > o) {
				if (e.match_start = t, o = a, a >= s) break;
				p = l[r + o - 1], m = l[r + o]
			}
		} while ((t = d[t & u]) > c && --n !== 0);
		return o <= e.lookahead ? o : e.lookahead
	},
	st = e => {
		let t = e.w_size,
			n, r, i;
		do {
			if (r = e.window_size - e.lookahead - e.strstart, e.strstart >= t + (t - L) && (e.window.set(e.window.subarray(t, t + t - r), 0), e.match_start -= t, e.strstart -= t, e.block_start -= t, e.insert > e.strstart && (e.insert = e.strstart), nt(e), r += t), e.strm.avail_in === 0) break;
			if (n = at(e.strm, e.window, e.strstart + e.lookahead, r), e.lookahead += n, !e.legacy_hash) {
				if (e.lookahead + e.insert > F)
					for (i = e.strstart - e.insert; e.insert && (G(e, i), i++, e.insert--, !(e.lookahead + e.insert <= F)););
			} else if (e.lookahead + e.insert >= F)
				for (i = e.strstart - e.insert, e.ins_h = e.window[i], e.ins_h = rt(e, e.ins_h, e.window[i + 1]); e.insert && (G(e, i), i++, e.insert--, !(e.lookahead + e.insert < F)););
		} while (e.lookahead < L && e.strm.avail_in !== 0)
	},
	ct = (e, t) => {
		let n = e.pending_buf_size - 5 > e.w_size ? e.w_size : e.pending_buf_size - 5,
			r, i, a, o = 0,
			s = e.strm.avail_in;
		do {
			if (r = 65535, a = e.bi_valid + 42 >> 3, e.strm.avail_out < a || (a = e.strm.avail_out - a, i = e.strstart - e.block_start, r > i + e.strm.avail_in && (r = i + e.strm.avail_in), r > a && (r = a), r < n && (r === 0 && t !== 4 || t === 0 || r !== i + e.strm.avail_in))) break;
			o = +(t === 4 && r === i + e.strm.avail_in), Se(e, 0, 0, o), e.pending_buf[e.pending - 4] = r, e.pending_buf[e.pending - 3] = r >> 8, e.pending_buf[e.pending - 2] = ~r, e.pending_buf[e.pending - 1] = ~r >> 8, K(e.strm), i && (i > r && (i = r), e.strm.output.set(e.window.subarray(e.block_start, e.block_start + i), e.strm.next_out), e.strm.next_out += i, e.strm.avail_out -= i, e.strm.total_out += i, e.block_start += i, r -= i), r && (at(e.strm, e.strm.output, e.strm.next_out, r), e.strm.next_out += r, e.strm.avail_out -= r, e.strm.total_out += r)
		} while (o === 0);
		return s -= e.strm.avail_in, s && (s >= e.w_size ? (e.matches = 2, e.window.set(e.strm.input.subarray(e.strm.next_in - e.w_size, e.strm.next_in), 0), e.strstart = e.w_size, e.insert = e.strstart) : (e.window_size - e.strstart <= s && (e.strstart -= e.w_size, e.window.set(e.window.subarray(e.w_size, e.w_size + e.strstart), 0), e.matches < 2 && e.matches++, e.insert > e.strstart && (e.insert = e.strstart)), e.window.set(e.strm.input.subarray(e.strm.next_in - s, e.strm.next_in), e.strstart), e.strstart += s, e.insert += s > e.w_size - e.insert ? e.w_size - e.insert : s), e.block_start = e.strstart), e.high_water < e.strstart && (e.high_water = e.strstart), o ? $e : t !== 0 && t !== 4 && e.strm.avail_in === 0 && e.strstart === e.block_start ? V : (a = e.window_size - e.strstart, e.strm.avail_in > a && e.block_start >= e.w_size && (e.block_start -= e.w_size, e.strstart -= e.w_size, e.window.set(e.window.subarray(e.w_size, e.w_size + e.strstart), 0), e.matches < 2 && e.matches++, a += e.w_size, e.insert > e.strstart && (e.insert = e.strstart)), a > e.strm.avail_in && (a = e.strm.avail_in), a && (at(e.strm, e.window, e.strstart, a), e.strstart += a, e.insert += a > e.w_size - e.insert ? e.w_size - e.insert : a), e.high_water < e.strstart && (e.high_water = e.strstart), a = e.bi_valid + 42 >> 3, a = e.pending_buf_size - a > 65535 ? 65535 : e.pending_buf_size - a, n = a > e.w_size ? e.w_size : a, i = e.strstart - e.block_start, (i >= n || (i || t === 4) && t !== 0 && e.strm.avail_in === 0 && i <= a) && (r = i > a ? a : i, o = +(t === 4 && e.strm.avail_in === 0 && r === i), Se(e, e.block_start, r, o), e.block_start += r, K(e.strm)), o ? H : B)
	},
	lt = (e, t) => {
		let n, r;
		for (;;) {
			if (e.lookahead < L) {
				if (st(e), e.lookahead < L && t === 0) return B;
				if (e.lookahead === 0) break
			}
			if (n = 0, e.lookahead >= F && (n = G(e, e.strstart)), n !== 0 && e.strstart - n <= e.w_size - L && (e.match_length = ot(e, n)), e.match_length >= F)
				if (r = M(e, e.strstart - e.match_start, e.match_length - F), e.lookahead -= e.match_length, e.match_length <= e.max_lazy_match && e.lookahead >= F) {
					e.match_length--;
					do e.strstart++, n = G(e, e.strstart); while (--e.match_length !== 0);
					e.strstart++
				} else e.strstart += e.match_length, e.match_length = 0, e.legacy_hash && (e.ins_h = e.window[e.strstart], e.ins_h = rt(e, e.ins_h, e.window[e.strstart + 1]));
			else r = M(e, 0, e.window[e.strstart]), e.lookahead--, e.strstart++;
			if (r && (q(e, !1), e.strm.avail_out === 0)) return B
		}
		return e.insert = e.strstart < F - 1 ? e.strstart : F - 1, t === 4 ? (q(e, !0), e.strm.avail_out === 0 ? H : $e) : e.sym_next && (q(e, !1), e.strm.avail_out === 0) ? B : V
	},
	ut = (e, t) => {
		let n, r, i;
		for (;;) {
			if (e.lookahead < L) {
				if (st(e), e.lookahead < L && t === 0) return B;
				if (e.lookahead === 0) break
			}
			if (n = 0, e.lookahead >= F && (n = G(e, e.strstart)), e.prev_length = e.match_length, e.prev_match = e.match_start, e.match_length = F - 1, n !== 0 && e.prev_length < e.max_lazy_match && e.strstart - n <= e.w_size - L && (e.match_length = ot(e, n), e.match_length <= 5 && (e.strategy === 1 || e.match_length === F && e.strstart - e.match_start > 4096) && (e.match_length = F - 1)), e.prev_length >= F && e.match_length <= e.prev_length) {
				i = e.strstart + e.lookahead - F, r = M(e, e.strstart - 1 - e.prev_match, e.prev_length - F), e.lookahead -= e.prev_length - 1, e.prev_length -= 2;
				do ++e.strstart <= i && (n = G(e, e.strstart)); while (--e.prev_length !== 0);
				if (e.match_available = 0, e.match_length = F - 1, e.strstart++, r && (q(e, !1), e.strm.avail_out === 0)) return B
			} else if (e.match_available) {
				if (r = M(e, 0, e.window[e.strstart - 1]), r && q(e, !1), e.strstart++, e.lookahead--, e.strm.avail_out === 0) return B
			} else e.match_available = 1, e.strstart++, e.lookahead--
		}
		return e.match_available && (r = M(e, 0, e.window[e.strstart - 1]), e.match_available = 0), e.insert = e.strstart < F - 1 ? e.strstart : F - 1, t === 4 ? (q(e, !0), e.strm.avail_out === 0 ? H : $e) : e.sym_next && (q(e, !1), e.strm.avail_out === 0) ? B : V
	},
	dt = (e, t) => {
		let n, r, i, a, o = e.window;
		for (;;) {
			if (e.lookahead <= I) {
				if (st(e), e.lookahead <= I && t === 0) return B;
				if (e.lookahead === 0) break
			}
			if (e.match_length = 0, e.lookahead >= F && e.strstart > 0 && (i = e.strstart - 1, r = o[i], r === o[++i] && r === o[++i] && r === o[++i])) {
				a = e.strstart + I;
				do; while (r === o[++i] && r === o[++i] && r === o[++i] && r === o[++i] && r === o[++i] && r === o[++i] && r === o[++i] && r === o[++i] && i < a);
				e.match_length = I - (a - i), e.match_length > e.lookahead && (e.match_length = e.lookahead)
			}
			if (e.match_length >= F ? (n = M(e, 1, e.match_length - F), e.lookahead -= e.match_length, e.strstart += e.match_length, e.match_length = 0) : (n = M(e, 0, e.window[e.strstart]), e.lookahead--, e.strstart++), n && (q(e, !1), e.strm.avail_out === 0)) return B
		}
		return e.insert = 0, t === 4 ? (q(e, !0), e.strm.avail_out === 0 ? H : $e) : e.sym_next && (q(e, !1), e.strm.avail_out === 0) ? B : V
	},
	ft = (e, t) => {
		let n;
		for (;;) {
			if (e.lookahead === 0 && (st(e), e.lookahead === 0)) {
				if (t === 0) return B;
				break
			}
			if (e.match_length = 0, n = M(e, 0, e.window[e.strstart]), e.lookahead--, e.strstart++, n && (q(e, !1), e.strm.avail_out === 0)) return B
		}
		return e.insert = 0, t === 4 ? (q(e, !0), e.strm.avail_out === 0 ? H : $e) : e.sym_next && (q(e, !1), e.strm.avail_out === 0) ? B : V
	},
	Y = class {
		constructor(e, t, n, r, i) {
			this.good_length = e, this.max_lazy = t, this.nice_length = n, this.max_chain = r, this.func = i
		}
	},
	pt = [new Y(0, 0, 0, 0, ct), new Y(4, 4, 8, 4, lt), new Y(4, 5, 16, 8, lt), new Y(4, 6, 32, 32, lt), new Y(4, 4, 16, 16, ut), new Y(8, 16, 32, 32, ut), new Y(8, 16, 128, 128, ut), new Y(8, 32, 128, 256, ut), new Y(32, 128, 258, 1024, ut), new Y(32, 258, 258, 4096, ut)],
	mt = e => {
		e.window_size = 2 * e.w_size, W(e.head), e.max_lazy_match = pt[e.level].max_lazy, e.good_match = pt[e.level].good_length, e.nice_match = pt[e.level].nice_length, e.max_chain_length = pt[e.level].max_chain, e.strstart = 0, e.block_start = 0, e.lookahead = 0, e.insert = 0, e.match_length = e.prev_length = F - 1, e.match_available = 0, e.ins_h = 0
	},
	ht = class {
		constructor() {
			this.strm = null, this.status = 0, this.pending_buf = null, this.pending_buf_size = 0, this.pending_out = 0, this.pending = 0, this.wrap = 0, this.gzhead = null, this.gzindex = 0, this.method = 8, this.last_flush = -1, this.w_size = 0, this.w_bits = 0, this.w_mask = 0, this.window = null, this.window_size = 0, this.prev = null, this.head = null, this.ins_h = 0, this.legacy_hash = 0, this.hash_size = 0, this.hash_bits = 0, this.hash_mask = 0, this.hash_shift = 0, this.block_start = 0, this.match_length = 0, this.prev_match = 0, this.match_available = 0, this.strstart = 0, this.match_start = 0, this.lookahead = 0, this.prev_length = 0, this.max_chain_length = 0, this.max_lazy_match = 0, this.level = 0, this.strategy = 0, this.good_match = 0, this.nice_match = 0, this.dyn_ltree = new Uint16Array(Ge * 2), this.dyn_dtree = new Uint16Array(122), this.bl_tree = new Uint16Array(78), W(this.dyn_ltree), W(this.dyn_dtree), W(this.bl_tree), this.l_desc = null, this.d_desc = null, this.bl_desc = null, this.bl_count = new Uint16Array(16), this.heap = new Uint16Array(573), W(this.heap), this.heap_len = 0, this.heap_max = 0, this.depth = new Uint16Array(573), W(this.depth), this.sym_buf = 0, this.lit_bufsize = 0, this.sym_next = 0, this.sym_end = 0, this.opt_len = 0, this.static_len = 0, this.matches = 0, this.insert = 0, this.bi_buf = 0, this.bi_valid = 0
		}
	},
	gt = e => {
		if (!e) return 1;
		let t = e.state;
		return +(!t || t.strm !== e || t.status !== R && t.status !== qe && t.status !== Je && t.status !== Ye && t.status !== Xe && t.status !== Ze && t.status !== z && t.status !== Qe)
	},
	_t = e => {
		if (gt(e)) return U(e, -2);
		e.total_in = e.total_out = 0, e.data_type = 2;
		let t = e.state;
		return t.pending = 0, t.pending_out = 0, t.wrap < 0 && (t.wrap = -t.wrap), t.status = t.wrap === 2 ? qe : t.wrap ? R : z, e.adler = t.wrap === 2 ? 0 : 1, t.last_flush = -2, xe(t), 0
	},
	vt = e => {
		let t = _t(e);
		return t === 0 && mt(e.state), t
	},
	yt = (e, t) => gt(e) || e.state.wrap !== 2 ? -2 : (e.state.gzhead = t, 0),
	bt = (e, t, n, r, i, a, o) => {
		if (!e) return -2;
		let s = 1;
		if (t === -1 && (t = 6), r < 0 ? (s = 0, r = -r) : r > 15 && (s = 2, r -= 16), i < 1 || i > He || n !== 8 || r < 8 || r > 15 || t < 0 || t > 9 || a < 0 || a > 4 || r === 8 && s !== 1) return U(e, -2);
		r === 8 && (r = 9);
		let c = new ht;
		return e.state = c, c.strm = e, c.status = R, c.wrap = s, c.gzhead = null, c.w_bits = r, c.w_size = 1 << c.w_bits, c.w_mask = c.w_size - 1, c.legacy_hash = +!!o, c.hash_bits = i + 7, !c.legacy_hash && c.hash_bits < 15 && (c.hash_bits = 15), c.hash_size = 1 << c.hash_bits, c.hash_mask = c.hash_size - 1, c.hash_shift = ~~((c.hash_bits + F - 1) / F), c.window = new Uint8Array(c.w_size * 2), c.head = new Uint16Array(c.hash_size), c.prev = new Uint16Array(c.w_size), c.lit_bufsize = 1 << i + 6, c.pending_buf_size = c.lit_bufsize * 4, c.pending_buf = new Uint8Array(c.pending_buf_size), c.sym_buf = c.lit_bufsize, c.sym_end = (c.lit_bufsize - 1) * 3, c.level = t, c.strategy = a, c.method = n, vt(e)
	},
	xt = (e, t) => bt(e, t, 8, Ue, We, 0),
	St = (e, t) => {
		if (gt(e) || t > 5 || t < 0) return e ? U(e, -2) : -2;
		let n = e.state;
		if (!e.output || e.avail_in !== 0 && !e.input || n.status === Qe && t !== 4) return U(e, e.avail_out === 0 ? -5 : -2);
		let r = n.last_flush;
		if (n.last_flush = t, n.pending !== 0) {
			if (K(e), e.avail_out === 0) return n.last_flush = -1, 0
		} else if (e.avail_in === 0 && tt(t) <= tt(r) && t !== 4) return U(e, -5);
		if (n.status === Qe && e.avail_in !== 0) return U(e, -5);
		if (n.status === R && n.wrap === 0 && (n.status = z), n.status === R) {
			let t = 8 + (n.w_bits - 8 << 4) << 8,
				r = -1;
			if (r = n.strategy >= 2 || n.level < 2 ? 0 : n.level < 6 ? 1 : n.level === 6 ? 2 : 3, t |= r << 6, n.strstart !== 0 && (t |= Ke), t += 31 - t % 31, it(n, t), n.strstart !== 0 && (it(n, e.adler >>> 16), it(n, e.adler & 65535)), e.adler = 1, n.status = z, K(e), n.pending !== 0) return n.last_flush = -1, 0
		}
		if (n.status === qe) {
			if (e.adler = 0, J(n, 31), J(n, 139), J(n, 8), n.gzhead) J(n, +!!n.gzhead.text + (n.gzhead.hcrc ? 2 : 0) + (n.gzhead.extra ? 4 : 0) + (n.gzhead.name ? 8 : 0) + (n.gzhead.comment ? 16 : 0)), J(n, n.gzhead.time & 255), J(n, n.gzhead.time >> 8 & 255), J(n, n.gzhead.time >> 16 & 255), J(n, n.gzhead.time >> 24 & 255), J(n, n.level === 9 ? 2 : n.strategy >= 2 || n.level < 2 ? 4 : 0), J(n, n.gzhead.os & 255), n.gzhead.extra && n.gzhead.extra.length && (J(n, n.gzhead.extra.length & 255), J(n, n.gzhead.extra.length >> 8 & 255)), n.gzhead.hcrc && (e.adler = N(e.adler, n.pending_buf, n.pending, 0)), n.gzindex = 0, n.status = Je;
			else if (J(n, 0), J(n, 0), J(n, 0), J(n, 0), J(n, 0), J(n, n.level === 9 ? 2 : n.strategy >= 2 || n.level < 2 ? 4 : 0), J(n, et), n.status = z, K(e), n.pending !== 0) return n.last_flush = -1, 0
		}
		if (n.status === Je) {
			if (n.gzhead.extra) {
				let t = n.pending,
					r = (n.gzhead.extra.length & 65535) - n.gzindex;
				for (; n.pending + r > n.pending_buf_size;) {
					let i = n.pending_buf_size - n.pending;
					if (n.pending_buf.set(n.gzhead.extra.subarray(n.gzindex, n.gzindex + i), n.pending), n.pending = n.pending_buf_size, n.gzhead.hcrc && n.pending > t && (e.adler = N(e.adler, n.pending_buf, n.pending - t, t)), n.gzindex += i, K(e), n.pending !== 0) return n.last_flush = -1, 0;
					t = 0, r -= i
				}
				let i = new Uint8Array(n.gzhead.extra);
				n.pending_buf.set(i.subarray(n.gzindex, n.gzindex + r), n.pending), n.pending += r, n.gzhead.hcrc && n.pending > t && (e.adler = N(e.adler, n.pending_buf, n.pending - t, t)), n.gzindex = 0
			}
			n.status = Ye
		}
		if (n.status === Ye) {
			if (n.gzhead.name) {
				let t = n.pending,
					r;
				do {
					if (n.pending === n.pending_buf_size) {
						if (n.gzhead.hcrc && n.pending > t && (e.adler = N(e.adler, n.pending_buf, n.pending - t, t)), K(e), n.pending !== 0) return n.last_flush = -1, 0;
						t = 0
					}
					r = n.gzindex < n.gzhead.name.length ? n.gzhead.name.charCodeAt(n.gzindex++) & 255 : 0, J(n, r)
				} while (r !== 0);
				n.gzhead.hcrc && n.pending > t && (e.adler = N(e.adler, n.pending_buf, n.pending - t, t)), n.gzindex = 0
			}
			n.status = Xe
		}
		if (n.status === Xe) {
			if (n.gzhead.comment) {
				let t = n.pending,
					r;
				do {
					if (n.pending === n.pending_buf_size) {
						if (n.gzhead.hcrc && n.pending > t && (e.adler = N(e.adler, n.pending_buf, n.pending - t, t)), K(e), n.pending !== 0) return n.last_flush = -1, 0;
						t = 0
					}
					r = n.gzindex < n.gzhead.comment.length ? n.gzhead.comment.charCodeAt(n.gzindex++) & 255 : 0, J(n, r)
				} while (r !== 0);
				n.gzhead.hcrc && n.pending > t && (e.adler = N(e.adler, n.pending_buf, n.pending - t, t))
			}
			n.status = Ze
		}
		if (n.status === Ze) {
			if (n.gzhead.hcrc) {
				if (n.pending + 2 > n.pending_buf_size && (K(e), n.pending !== 0)) return n.last_flush = -1, 0;
				J(n, e.adler & 255), J(n, e.adler >> 8 & 255), e.adler = 0
			}
			if (n.status = z, K(e), n.pending !== 0) return n.last_flush = -1, 0
		}
		if (e.avail_in !== 0 || n.lookahead !== 0 || t !== 0 && n.status !== Qe) {
			let r = n.level === 0 ? ct(n, t) : n.strategy === 2 ? ft(n, t) : n.strategy === 3 ? dt(n, t) : pt[n.level].func(n, t);
			if ((r === H || r === $e) && (n.status = Qe), r === B || r === H) return e.avail_out === 0 && (n.last_flush = -1), 0;
			if (r === V && (t === 1 ? Ce(n) : t !== 5 && (Se(n, 0, 0, !1), t === 3 && (W(n.head), n.lookahead === 0 && (n.strstart = 0, n.block_start = 0, n.insert = 0))), K(e), e.avail_out === 0)) return n.last_flush = -1, 0
		}
		return t === 4 ? n.wrap <= 0 ? 1 : (n.wrap === 2 ? (J(n, e.adler & 255), J(n, e.adler >> 8 & 255), J(n, e.adler >> 16 & 255), J(n, e.adler >> 24 & 255), J(n, e.total_in & 255), J(n, e.total_in >> 8 & 255), J(n, e.total_in >> 16 & 255), J(n, e.total_in >> 24 & 255)) : (it(n, e.adler >>> 16), it(n, e.adler & 65535)), K(e), n.wrap > 0 && (n.wrap = -n.wrap), +(n.pending === 0)) : 0
	},
	Ct = e => {
		if (gt(e)) return -2;
		let t = e.state.status;
		return e.state = null, t === z ? U(e, -3) : 0
	},
	wt = (e, t) => {
		let n = t.length;
		if (gt(e)) return -2;
		let r = e.state,
			i = r.wrap;
		if (i === 2 || i === 1 && r.status !== R || r.lookahead) return -2;
		if (i === 1 && (e.adler = Te(e.adler, t, n, 0)), r.wrap = 0, n >= r.w_size) {
			i === 0 && (W(r.head), r.strstart = 0, r.block_start = 0, r.insert = 0);
			let e = new Uint8Array(r.w_size);
			e.set(t.subarray(n - r.w_size, n), 0), t = e, n = r.w_size
		}
		let a = e.avail_in,
			o = e.next_in,
			s = e.input;
		for (e.avail_in = n, e.next_in = 0, e.input = t, st(r); r.lookahead >= F;) {
			let e = r.strstart,
				t = r.lookahead - (F - 1);
			do G(r, e), e++; while (--t);
			r.strstart = e, r.lookahead = F - 1, st(r)
		}
		return r.strstart += r.lookahead, r.block_start = r.strstart, r.insert = r.lookahead, r.lookahead = 0, r.match_length = r.prev_length = F - 1, r.match_available = 0, e.next_in = o, e.input = s, e.avail_in = a, r.wrap = i, 0
	},
	Tt = 16209,
	Et = 16191;

function Dt(e, t) {
	let n, r, i, a, o, s, c, l, u, d, f, p, m, h, g, _, v, y, b, x, S, C, w, T, E = e.state;
	n = e.next_in, w = e.input, r = n + (e.avail_in - 5), i = e.next_out, T = e.output, a = i - (t - e.avail_out), o = i + (e.avail_out - 257), s = E.dmax, c = E.wsize, l = E.whave, u = E.wnext, d = E.window, f = E.hold, p = E.bits, m = E.lencode, h = E.distcode, g = (1 << E.lenbits) - 1, _ = (1 << E.distbits) - 1;
	top: do {
		p < 15 && (f += w[n++] << p, p += 8, f += w[n++] << p, p += 8), v = m[f & g];
		dolen: for (;;) {
			if (y = v >>> 24, f >>>= y, p -= y, y = v >>> 16 & 255, y === 0) T[i++] = v & 65535;
			else if (y & 16) {
				b = v & 65535, y &= 15, y && (p < y && (f += w[n++] << p, p += 8), b += f & (1 << y) - 1, f >>>= y, p -= y), p < 15 && (f += w[n++] << p, p += 8, f += w[n++] << p, p += 8), v = h[f & _];
				dodist: for (;;) {
					if (y = v >>> 24, f >>>= y, p -= y, y = v >>> 16 & 255, y & 16) {
						if (x = v & 65535, y &= 15, p < y && (f += w[n++] << p, p += 8, p < y && (f += w[n++] << p, p += 8)), x += f & (1 << y) - 1, x > s) {
							e.msg = `invalid distance too far back`, E.mode = Tt;
							break top
						}
						if (f >>>= y, p -= y, y = i - a, x > y) {
							if (y = x - y, y > l && E.sane) {
								e.msg = `invalid distance too far back`, E.mode = Tt;
								break top
							}
							if (S = 0, C = d, u === 0) {
								if (S += c - y, y < b) {
									b -= y;
									do T[i++] = d[S++]; while (--y);
									S = i - x, C = T
								}
							} else if (u < y) {
								if (S += c + u - y, y -= u, y < b) {
									b -= y;
									do T[i++] = d[S++]; while (--y);
									if (S = 0, u < b) {
										y = u, b -= y;
										do T[i++] = d[S++]; while (--y);
										S = i - x, C = T
									}
								}
							} else if (S += u - y, y < b) {
								b -= y;
								do T[i++] = d[S++]; while (--y);
								S = i - x, C = T
							}
							for (; b > 2;) T[i++] = C[S++], T[i++] = C[S++], T[i++] = C[S++], b -= 3;
							b && (T[i++] = C[S++], b > 1 && (T[i++] = C[S++]))
						} else {
							S = i - x;
							do T[i++] = T[S++], T[i++] = T[S++], T[i++] = T[S++], b -= 3; while (b > 2);
							b && (T[i++] = T[S++], b > 1 && (T[i++] = T[S++]))
						}
					} else if (y & 64) {
						e.msg = `invalid distance code`, E.mode = Tt;
						break top
					} else {
						v = h[(v & 65535) + (f & (1 << y) - 1)];
						continue dodist
					}
					break
				}
			} else if (!(y & 64)) {
				v = m[(v & 65535) + (f & (1 << y) - 1)];
				continue dolen
			} else if (y & 32) {
				E.mode = Et;
				break top
			} else {
				e.msg = `invalid literal/length code`, E.mode = Tt;
				break top
			}
			break
		}
	} while (n < r && i < o);
	b = p >> 3, n -= b, p -= b << 3, f &= (1 << p) - 1, e.next_in = n, e.next_out = i, e.avail_in = n < r ? 5 + (r - n) : 5 - (n - r), e.avail_out = i < o ? 257 + (o - i) : 257 - (i - o), E.hold = f, E.bits = p
}
var Ot = 15,
	kt = 852,
	At = 592,
	jt = 0,
	Mt = 1,
	Nt = 2,
	Pt = new Uint16Array([3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0]),
	Ft = new Uint8Array([16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18, 19, 19, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 16, 199, 75]),
	It = new Uint16Array([1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 0, 0]),
	Lt = new Uint8Array([16, 16, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 64, 64]),
	Rt = (e, t, n, r, i, a, o, s) => {
		let c = s.bits,
			l = 0,
			u = 0,
			d = 0,
			f = 0,
			p = 0,
			m = 0,
			h = 0,
			g = 0,
			_ = 0,
			v = 0,
			y, b, x, S, C, w = null,
			T, E = new Uint16Array(16),
			D = new Uint16Array(16),
			O = null,
			ee, te, k;
		for (l = 0; l <= Ot; l++) E[l] = 0;
		for (u = 0; u < r; u++) E[t[n + u]]++;
		for (p = c, f = Ot; f >= 1 && E[f] === 0; f--);
		if (p > f && (p = f), f === 0) return i[a++] = 20971520, i[a++] = 20971520, s.bits = 1, 0;
		for (d = 1; d < f && E[d] === 0; d++);
		for (p < d && (p = d), g = 1, l = 1; l <= Ot; l++)
			if (g <<= 1, g -= E[l], g < 0) return -1;
		if (g > 0 && (e === jt || f !== 1)) return -1;
		for (D[1] = 0, l = 1; l < Ot; l++) D[l + 1] = D[l] + E[l];
		for (u = 0; u < r; u++) t[n + u] !== 0 && (o[D[t[n + u]]++] = u);
		if (e === jt ? (w = O = o, T = 20) : e === Mt ? (w = Pt, O = Ft, T = 257) : (w = It, O = Lt, T = 0), v = 0, u = 0, l = d, C = a, m = p, h = 0, x = -1, _ = 1 << p, S = _ - 1, e === Mt && _ > kt || e === Nt && _ > At) return 1;
		for (;;) {
			ee = l - h, o[u] + 1 < T ? (te = 0, k = o[u]) : o[u] >= T ? (te = O[o[u] - T], k = w[o[u] - T]) : (te = 96, k = 0), y = 1 << l - h, b = 1 << m, d = b;
			do b -= y, i[C + (v >> h) + b] = ee << 24 | te << 16 | k | 0; while (b !== 0);
			for (y = 1 << l - 1; v & y;) y >>= 1;
			if (y === 0 ? v = 0 : (v &= y - 1, v += y), u++, --E[l] === 0) {
				if (l === f) break;
				l = t[n + o[u]]
			}
			if (l > p && (v & S) !== x) {
				for (h === 0 && (h = p), C += d, m = l - h, g = 1 << m; m + h < f && (g -= E[m + h], !(g <= 0));) m++, g <<= 1;
				if (_ += 1 << m, e === Mt && _ > kt || e === Nt && _ > At) return 1;
				x = v & S, i[x] = p << 24 | m << 16 | C - a | 0
			}
		}
		return v !== 0 && (i[C + v] = l - h << 24 | 4194304), s.bits = p, 0
	},
	zt = 0,
	Bt = 1,
	Vt = 2,
	Ht = 16180,
	Ut = 16181,
	Wt = 16182,
	Gt = 16183,
	Kt = 16184,
	qt = 16185,
	Jt = 16186,
	Yt = 16187,
	Xt = 16188,
	Zt = 16189,
	Qt = 16190,
	X = 16191,
	$t = 16192,
	en = 16193,
	tn = 16194,
	nn = 16195,
	rn = 16196,
	an = 16197,
	on = 16198,
	sn = 16199,
	cn = 16200,
	ln = 16201,
	un = 16202,
	dn = 16203,
	fn = 16204,
	pn = 16205,
	mn = 16206,
	hn = 16207,
	gn = 16208,
	Z = 16209,
	_n = 16210,
	vn = 16211,
	yn = 852,
	bn = 592,
	xn = 15,
	Sn = e => (e >>> 24 & 255) + (e >>> 8 & 65280) + ((e & 65280) << 8) + ((e & 255) << 24),
	Cn = class {
		constructor() {
			this.strm = null, this.mode = 0, this.last = !1, this.wrap = 0, this.havedict = !1, this.flags = 0, this.dmax = 0, this.check = 0, this.total = 0, this.head = null, this.wbits = 0, this.wsize = 0, this.whave = 0, this.wnext = 0, this.window = null, this.hold = 0, this.bits = 0, this.length = 0, this.offset = 0, this.extra = 0, this.lencode = null, this.distcode = null, this.lenbits = 0, this.distbits = 0, this.ncode = 0, this.nlen = 0, this.ndist = 0, this.have = 0, this.next = null, this.lens = new Uint16Array(320), this.work = new Uint16Array(288), this.lendyn = null, this.distdyn = null, this.sane = 0, this.back = 0, this.was = 0
		}
	},
	Q = e => {
		if (!e) return 1;
		let t = e.state;
		return +(!t || t.strm !== e || t.mode < Ht || t.mode > vn)
	},
	wn = e => {
		if (Q(e)) return -2;
		let t = e.state;
		return e.total_in = e.total_out = t.total = 0, e.msg = ``, t.wrap && (e.adler = t.wrap & 1), t.mode = Ht, t.last = 0, t.havedict = 0, t.flags = -1, t.dmax = 32768, t.head = null, t.hold = 0, t.bits = 0, t.lencode = t.lendyn = new Int32Array(yn), t.distcode = t.distdyn = new Int32Array(bn), t.sane = 1, t.back = -1, 0
	},
	Tn = e => {
		if (Q(e)) return -2;
		let t = e.state;
		return t.wsize = 0, t.whave = 0, t.wnext = 0, wn(e)
	},
	En = (e, t) => {
		let n;
		if (Q(e)) return -2;
		let r = e.state;
		return t < 0 ? (n = 0, t = -t) : (n = (t >> 4) + 5, t < 48 && (t &= 15)), t && (t < 8 || t > 15) ? -2 : (r.window !== null && r.wbits !== t && (r.window = null), r.wrap = n, r.wbits = t, Tn(e))
	},
	Dn = (e, t) => {
		if (!e) return -2;
		let n = new Cn;
		e.state = n, n.strm = e, n.window = null, n.mode = Ht;
		let r = En(e, t);
		return r !== 0 && (e.state = null), r
	},
	On = e => Dn(e, xn),
	kn = !0,
	An, jn, Mn = e => {
		if (kn) {
			An = new Int32Array(512), jn = new Int32Array(32);
			let t = 0;
			for (; t < 144;) e.lens[t++] = 8;
			for (; t < 256;) e.lens[t++] = 9;
			for (; t < 280;) e.lens[t++] = 7;
			for (; t < 288;) e.lens[t++] = 8;
			for (Rt(Bt, e.lens, 0, 288, An, 0, e.work, {
					bits: 9
				}), t = 0; t < 32;) e.lens[t++] = 5;
			Rt(Vt, e.lens, 0, 32, jn, 0, e.work, {
				bits: 5
			}), kn = !1
		}
		e.lencode = An, e.lenbits = 9, e.distcode = jn, e.distbits = 5
	},
	Nn = (e, t, n, r) => {
		let i, a = e.state;
		return a.window === null && (a.window = new Uint8Array(1 << a.wbits)), a.wsize === 0 && (a.wsize = 1 << a.wbits, a.wnext = 0, a.whave = 0), r >= a.wsize ? (a.window.set(t.subarray(n - a.wsize, n), 0), a.wnext = 0, a.whave = a.wsize) : (i = a.wsize - a.wnext, i > r && (i = r), a.window.set(t.subarray(n - r, n - r + i), a.wnext), r -= i, r ? (a.window.set(t.subarray(n - r, n), 0), a.wnext = r, a.whave = a.wsize) : (a.wnext += i, a.wnext === a.wsize && (a.wnext = 0), a.whave < a.wsize && (a.whave += i))), 0
	},
	Pn = (e, t) => {
		let n, r, i, a, o, s, c, l, u, d, f, p, m, h, g = 0,
			_, v, y, b, x, S, C, w, T = new Uint8Array(4),
			E, D, O = new Uint8Array([16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15]);
		if (Q(e) || !e.output || !e.input && e.avail_in !== 0) return -2;
		n = e.state, n.mode === X && (n.mode = $t), o = e.next_out, i = e.output, c = e.avail_out, a = e.next_in, r = e.input, s = e.avail_in, l = n.hold, u = n.bits, d = s, f = c, w = 0;
		inf_leave: for (;;) switch (n.mode) {
			case Ht:
				if (n.wrap === 0) {
					n.mode = $t;
					break
				}
				for (; u < 16;) {
					if (s === 0) break inf_leave;
					s--, l += r[a++] << u, u += 8
				}
				if (n.wrap & 2 && l === 35615) {
					n.wbits === 0 && (n.wbits = 15), n.check = 0, T[0] = l & 255, T[1] = l >>> 8 & 255, n.check = N(n.check, T, 2, 0), l = 0, u = 0, n.mode = Ut;
					break
				}
				if (n.head && (n.head.done = !1), !(n.wrap & 1) || (((l & 255) << 8) + (l >> 8)) % 31) {
					e.msg = `incorrect header check`, n.mode = Z;
					break
				}
				if ((l & 15) != 8) {
					e.msg = `unknown compression method`, n.mode = Z;
					break
				}
				if (l >>>= 4, u -= 4, C = (l & 15) + 8, n.wbits === 0 && (n.wbits = C), C > 15 || C > n.wbits) {
					e.msg = `invalid window size`, n.mode = Z;
					break
				}
				n.dmax = 1 << n.wbits, n.flags = 0, e.adler = n.check = 1, n.mode = l & 512 ? Zt : X, l = 0, u = 0;
				break;
			case Ut:
				for (; u < 16;) {
					if (s === 0) break inf_leave;
					s--, l += r[a++] << u, u += 8
				}
				if (n.flags = l, (n.flags & 255) != 8) {
					e.msg = `unknown compression method`, n.mode = Z;
					break
				}
				if (n.flags & 57344) {
					e.msg = `unknown header flags set`, n.mode = Z;
					break
				}
				n.head && (n.head.text = l >> 8 & 1), n.flags & 512 && n.wrap & 4 && (T[0] = l & 255, T[1] = l >>> 8 & 255, n.check = N(n.check, T, 2, 0)), l = 0, u = 0, n.mode = Wt;
			case Wt:
				for (; u < 32;) {
					if (s === 0) break inf_leave;
					s--, l += r[a++] << u, u += 8
				}
				n.head && (n.head.time = l), n.flags & 512 && n.wrap & 4 && (T[0] = l & 255, T[1] = l >>> 8 & 255, T[2] = l >>> 16 & 255, T[3] = l >>> 24 & 255, n.check = N(n.check, T, 4, 0)), l = 0, u = 0, n.mode = Gt;
			case Gt:
				for (; u < 16;) {
					if (s === 0) break inf_leave;
					s--, l += r[a++] << u, u += 8
				}
				n.head && (n.head.xflags = l & 255, n.head.os = l >> 8), n.flags & 512 && n.wrap & 4 && (T[0] = l & 255, T[1] = l >>> 8 & 255, n.check = N(n.check, T, 2, 0)), l = 0, u = 0, n.mode = Kt;
			case Kt:
				if (n.flags & 1024) {
					for (; u < 16;) {
						if (s === 0) break inf_leave;
						s--, l += r[a++] << u, u += 8
					}
					n.length = l, n.head && (n.head.extra_len = l), n.flags & 512 && n.wrap & 4 && (T[0] = l & 255, T[1] = l >>> 8 & 255, n.check = N(n.check, T, 2, 0)), l = 0, u = 0
				} else n.head && (n.head.extra = null);
				n.mode = qt;
			case qt:
				if (n.flags & 1024 && (p = n.length, p > s && (p = s), p && (n.head && (C = n.head.extra_len - n.length, n.head.extra || (n.head.extra = new Uint8Array(n.head.extra_len)), n.head.extra.set(r.subarray(a, a + p), C)), n.flags & 512 && n.wrap & 4 && (n.check = N(n.check, r, p, a)), s -= p, a += p, n.length -= p), n.length)) break inf_leave;
				n.length = 0, n.mode = Jt;
			case Jt:
				if (n.flags & 2048) {
					if (s === 0) break inf_leave;
					p = 0;
					do C = r[a + p++], n.head && C && n.length < 65536 && (n.head.name += String.fromCharCode(C)); while (C && p < s);
					if (n.flags & 512 && n.wrap & 4 && (n.check = N(n.check, r, p, a)), s -= p, a += p, C) break inf_leave
				} else n.head && (n.head.name = null);
				n.length = 0, n.mode = Yt;
			case Yt:
				if (n.flags & 4096) {
					if (s === 0) break inf_leave;
					p = 0;
					do C = r[a + p++], n.head && C && n.length < 65536 && (n.head.comment += String.fromCharCode(C)); while (C && p < s);
					if (n.flags & 512 && n.wrap & 4 && (n.check = N(n.check, r, p, a)), s -= p, a += p, C) break inf_leave
				} else n.head && (n.head.comment = null);
				n.mode = Xt;
			case Xt:
				if (n.flags & 512) {
					for (; u < 16;) {
						if (s === 0) break inf_leave;
						s--, l += r[a++] << u, u += 8
					}
					if (n.wrap & 4 && l !== (n.check & 65535)) {
						e.msg = `header crc mismatch`, n.mode = Z;
						break
					}
					l = 0, u = 0
				}
				n.head && (n.head.hcrc = n.flags >> 9 & 1, n.head.done = !0), e.adler = n.check = 0, n.mode = X;
				break;
			case Zt:
				for (; u < 32;) {
					if (s === 0) break inf_leave;
					s--, l += r[a++] << u, u += 8
				}
				e.adler = n.check = Sn(l), l = 0, u = 0, n.mode = Qt;
			case Qt:
				if (n.havedict === 0) return e.next_out = o, e.avail_out = c, e.next_in = a, e.avail_in = s, n.hold = l, n.bits = u, 2;
				e.adler = n.check = 1, n.mode = X;
			case X:
				if (t === 5 || t === 6) break inf_leave;
			case $t:
				if (n.last) {
					l >>>= u & 7, u -= u & 7, n.mode = mn;
					break
				}
				for (; u < 3;) {
					if (s === 0) break inf_leave;
					s--, l += r[a++] << u, u += 8
				}
				switch (n.last = l & 1, l >>>= 1, --u, l & 3) {
					case 0:
						n.mode = en;
						break;
					case 1:
						if (Mn(n), n.mode = sn, t === 6) {
							l >>>= 2, u -= 2;
							break inf_leave
						}
						break;
					case 2:
						n.mode = rn;
						break;
					case 3:
						e.msg = `invalid block type`, n.mode = Z
				}
				l >>>= 2, u -= 2;
				break;
			case en:
				for (l >>>= u & 7, u -= u & 7; u < 32;) {
					if (s === 0) break inf_leave;
					s--, l += r[a++] << u, u += 8
				}
				if ((l & 65535) != (l >>> 16 ^ 65535)) {
					e.msg = `invalid stored block lengths`, n.mode = Z;
					break
				}
				if (n.length = l & 65535, l = 0, u = 0, n.mode = tn, t === 6) break inf_leave;
			case tn:
				n.mode = nn;
			case nn:
				if (p = n.length, p) {
					if (p > s && (p = s), p > c && (p = c), p === 0) break inf_leave;
					i.set(r.subarray(a, a + p), o), s -= p, a += p, c -= p, o += p, n.length -= p;
					break
				}
				n.mode = X;
				break;
			case rn:
				for (; u < 14;) {
					if (s === 0) break inf_leave;
					s--, l += r[a++] << u, u += 8
				}
				if (n.nlen = (l & 31) + 257, l >>>= 5, u -= 5, n.ndist = (l & 31) + 1, l >>>= 5, u -= 5, n.ncode = (l & 15) + 4, l >>>= 4, u -= 4, n.nlen > 286 || n.ndist > 30) {
					e.msg = `too many length or distance symbols`, n.mode = Z;
					break
				}
				n.have = 0, n.mode = an;
			case an:
				for (; n.have < n.ncode;) {
					for (; u < 3;) {
						if (s === 0) break inf_leave;
						s--, l += r[a++] << u, u += 8
					}
					n.lens[O[n.have++]] = l & 7, l >>>= 3, u -= 3
				}
				for (; n.have < 19;) n.lens[O[n.have++]] = 0;
				if (n.lencode = n.lendyn, n.lenbits = 7, E = {
						bits: n.lenbits
					}, w = Rt(zt, n.lens, 0, 19, n.lencode, 0, n.work, E), n.lenbits = E.bits, w) {
					e.msg = `invalid code lengths set`, n.mode = Z;
					break
				}
				n.have = 0, n.mode = on;
			case on:
				for (; n.have < n.nlen + n.ndist;) {
					for (; g = n.lencode[l & (1 << n.lenbits) - 1], _ = g >>> 24, v = g >>> 16 & 255, y = g & 65535, !(_ <= u);) {
						if (s === 0) break inf_leave;
						s--, l += r[a++] << u, u += 8
					}
					if (y < 16) l >>>= _, u -= _, n.lens[n.have++] = y;
					else {
						if (y === 16) {
							for (D = _ + 2; u < D;) {
								if (s === 0) break inf_leave;
								s--, l += r[a++] << u, u += 8
							}
							if (l >>>= _, u -= _, n.have === 0) {
								e.msg = `invalid bit length repeat`, n.mode = Z;
								break
							}
							C = n.lens[n.have - 1], p = 3 + (l & 3), l >>>= 2, u -= 2
						} else if (y === 17) {
							for (D = _ + 3; u < D;) {
								if (s === 0) break inf_leave;
								s--, l += r[a++] << u, u += 8
							}
							l >>>= _, u -= _, C = 0, p = 3 + (l & 7), l >>>= 3, u -= 3
						} else {
							for (D = _ + 7; u < D;) {
								if (s === 0) break inf_leave;
								s--, l += r[a++] << u, u += 8
							}
							l >>>= _, u -= _, C = 0, p = 11 + (l & 127), l >>>= 7, u -= 7
						}
						if (n.have + p > n.nlen + n.ndist) {
							e.msg = `invalid bit length repeat`, n.mode = Z;
							break
						}
						for (; p--;) n.lens[n.have++] = C
					}
				}
				if (n.mode === Z) break;
				if (n.lens[256] === 0) {
					e.msg = `invalid code -- missing end-of-block`, n.mode = Z;
					break
				}
				if (n.lenbits = 9, E = {
						bits: n.lenbits
					}, w = Rt(Bt, n.lens, 0, n.nlen, n.lencode, 0, n.work, E), n.lenbits = E.bits, w) {
					e.msg = `invalid literal/lengths set`, n.mode = Z;
					break
				}
				if (n.distbits = 6, n.distcode = n.distdyn, E = {
						bits: n.distbits
					}, w = Rt(Vt, n.lens, n.nlen, n.ndist, n.distcode, 0, n.work, E), n.distbits = E.bits, w) {
					e.msg = `invalid distances set`, n.mode = Z;
					break
				}
				if (n.mode = sn, t === 6) break inf_leave;
			case sn:
				n.mode = cn;
			case cn:
				if (s >= 6 && c >= 258) {
					e.next_out = o, e.avail_out = c, e.next_in = a, e.avail_in = s, n.hold = l, n.bits = u, Dt(e, f), o = e.next_out, i = e.output, c = e.avail_out, a = e.next_in, r = e.input, s = e.avail_in, l = n.hold, u = n.bits, n.mode === X && (n.back = -1);
					break
				}
				for (n.back = 0; g = n.lencode[l & (1 << n.lenbits) - 1], _ = g >>> 24, v = g >>> 16 & 255, y = g & 65535, !(_ <= u);) {
					if (s === 0) break inf_leave;
					s--, l += r[a++] << u, u += 8
				}
				if (v && !(v & 240)) {
					for (b = _, x = v, S = y; g = n.lencode[S + ((l & (1 << b + x) - 1) >> b)], _ = g >>> 24, v = g >>> 16 & 255, y = g & 65535, !(b + _ <= u);) {
						if (s === 0) break inf_leave;
						s--, l += r[a++] << u, u += 8
					}
					l >>>= b, u -= b, n.back += b
				}
				if (l >>>= _, u -= _, n.back += _, n.length = y, v === 0) {
					n.mode = pn;
					break
				}
				if (v & 32) {
					n.back = -1, n.mode = X;
					break
				}
				if (v & 64) {
					e.msg = `invalid literal/length code`, n.mode = Z;
					break
				}
				n.extra = v & 15, n.mode = ln;
			case ln:
				if (n.extra) {
					for (D = n.extra; u < D;) {
						if (s === 0) break inf_leave;
						s--, l += r[a++] << u, u += 8
					}
					n.length += l & (1 << n.extra) - 1, l >>>= n.extra, u -= n.extra, n.back += n.extra
				}
				n.was = n.length, n.mode = un;
			case un:
				for (; g = n.distcode[l & (1 << n.distbits) - 1], _ = g >>> 24, v = g >>> 16 & 255, y = g & 65535, !(_ <= u);) {
					if (s === 0) break inf_leave;
					s--, l += r[a++] << u, u += 8
				}
				if (!(v & 240)) {
					for (b = _, x = v, S = y; g = n.distcode[S + ((l & (1 << b + x) - 1) >> b)], _ = g >>> 24, v = g >>> 16 & 255, y = g & 65535, !(b + _ <= u);) {
						if (s === 0) break inf_leave;
						s--, l += r[a++] << u, u += 8
					}
					l >>>= b, u -= b, n.back += b
				}
				if (l >>>= _, u -= _, n.back += _, v & 64) {
					e.msg = `invalid distance code`, n.mode = Z;
					break
				}
				n.offset = y, n.extra = v & 15, n.mode = dn;
			case dn:
				if (n.extra) {
					for (D = n.extra; u < D;) {
						if (s === 0) break inf_leave;
						s--, l += r[a++] << u, u += 8
					}
					n.offset += l & (1 << n.extra) - 1, l >>>= n.extra, u -= n.extra, n.back += n.extra
				}
				if (n.offset > n.dmax) {
					e.msg = `invalid distance too far back`, n.mode = Z;
					break
				}
				n.mode = fn;
			case fn:
				if (c === 0) break inf_leave;
				if (p = f - c, n.offset > p) {
					if (p = n.offset - p, p > n.whave && n.sane) {
						e.msg = `invalid distance too far back`, n.mode = Z;
						break
					}
					p > n.wnext ? (p -= n.wnext, m = n.wsize - p) : m = n.wnext - p, p > n.length && (p = n.length), h = n.window
				} else h = i, m = o - n.offset, p = n.length;
				p > c && (p = c), c -= p, n.length -= p;
				do i[o++] = h[m++]; while (--p);
				n.length === 0 && (n.mode = cn);
				break;
			case pn:
				if (c === 0) break inf_leave;
				i[o++] = n.length, c--, n.mode = cn;
				break;
			case mn:
				if (n.wrap) {
					for (; u < 32;) {
						if (s === 0) break inf_leave;
						s--, l |= r[a++] << u, u += 8
					}
					if (f -= c, e.total_out += f, n.total += f, n.wrap & 4 && f && (e.adler = n.check = n.flags ? N(n.check, i, f, o - f) : Te(n.check, i, f, o - f)), f = c, n.wrap & 4 && (n.flags ? l : Sn(l)) !== n.check) {
						e.msg = `incorrect data check`, n.mode = Z;
						break
					}
					l = 0, u = 0
				}
				n.mode = hn;
			case hn:
				if (n.wrap && n.flags) {
					for (; u < 32;) {
						if (s === 0) break inf_leave;
						s--, l += r[a++] << u, u += 8
					}
					if (n.wrap & 4 && l !== (n.total & 4294967295)) {
						e.msg = `incorrect length check`, n.mode = Z;
						break
					}
					l = 0, u = 0
				}
				n.mode = gn;
			case gn:
				w = 1;
				break inf_leave;
			case Z:
				w = -3;
				break inf_leave;
			case _n:
				return -4;
			case vn:
			default:
				return -2
		}
		return e.next_out = o, e.avail_out = c, e.next_in = a, e.avail_in = s, n.hold = l, n.bits = u, (n.wsize || f !== e.avail_out && n.mode < Z && (n.mode < mn || t !== 4)) && Nn(e, e.output, e.next_out, f - e.avail_out) ? (n.mode = _n, -4) : (d -= e.avail_in, f -= e.avail_out, e.total_in += d, e.total_out += f, n.total += f, n.wrap & 4 && f && (e.adler = n.check = n.flags ? N(n.check, i, f, e.next_out - f) : Te(n.check, i, f, e.next_out - f)), e.data_type = n.bits + (n.last ? 64 : 0) + (n.mode === X ? 128 : 0) + (n.mode === sn || n.mode === tn ? 256 : 0), (d === 0 && f === 0 || t === 4) && w === 0 && (w = -5), w)
	},
	Fn = e => {
		if (Q(e)) return -2;
		let t = e.state;
		return t.window && (t.window = null), e.state = null, 0
	},
	In = (e, t) => {
		if (Q(e)) return -2;
		let n = e.state;
		return n.wrap & 2 ? (n.head = t, t.done = !1, 0) : -2
	},
	Ln = (e, t) => {
		let n = t.length,
			r, i, a;
		return Q(e) || (r = e.state, r.wrap !== 0 && r.mode !== Qt) ? -2 : r.mode === Qt && (i = 1, i = Te(i, t, n, 0), i !== r.check) ? -3 : (a = Nn(e, t, n, n), a ? (r.mode = _n, -4) : (r.havedict = 1, 0))
	},
	Rn = class {
		constructor() {
			this.input = null, this.next_in = 0, this.avail_in = 0, this.total_in = 0, this.output = null, this.next_out = 0, this.avail_out = 0, this.total_out = 0, this.msg = ``, this.state = null, this.data_type = 2, this.adler = 0
		}
	},
	zn = class {
		constructor() {
			this.text = 0, this.time = 0, this.xflags = 0, this.os = 0, this.extra = null, this.extra_len = 0, this.name = ``, this.comment = ``, this.hcrc = 0, this.done = !1
		}
	},
	Bn = e => {
		let t = new Uint8Array(e.reduce((e, t) => e + t.length, 0)),
			n = 0;
		for (let r of e) t.set(r, n), n += r.length;
		return t
	};

function Vn(e) {
	"@babel/helpers - typeof";
	return Vn = typeof Symbol == `function` && typeof Symbol.iterator == `symbol` ? function(e) {
		return typeof e
	} : function(e) {
		return e && typeof Symbol == `function` && e.constructor === Symbol && e !== Symbol.prototype ? `symbol` : typeof e
	}, Vn(e)
}

function Hn(e, t) {
	if (Vn(e) != `object` || !e) return e;
	var n = e[Symbol.toPrimitive];
	if (n !== void 0) {
		var r = n.call(e, t || `default`);
		if (Vn(r) != `object`) return r;
		throw TypeError(`@@toPrimitive must return a primitive value.`)
	}
	return (t === `string` ? String : Number)(e)
}

function Un(e) {
	var t = Hn(e, `string`);
	return Vn(t) == `symbol` ? t : t + ``
}

function $(e, t, n) {
	return (t = Un(t)) in e ? Object.defineProperty(e, t, {
		value: n,
		enumerable: !0,
		configurable: !0,
		writable: !0
	}) : e[t] = n, e
}
var Wn = Object.prototype.toString,
	Gn = {
		level: -1,
		chunkSize: 16384,
		windowBits: 15,
		memLevel: 8,
		strategy: 0,
		raw: !1,
		gzip: !1,
		legacyHash: !1,
		dictionary: new Uint8Array
	},
	Kn = class {
		constructor(e = {}) {
			$(this, `options`, void 0), $(this, `err`, void 0), $(this, `msg`, void 0), $(this, `ended`, void 0), $(this, `started`, void 0), $(this, `chunks`, void 0), $(this, `strm`, void 0), $(this, `result`, void 0), this.options = Object.assign({}, Gn, e);
			let t = this.options;
			t.raw && t.windowBits > 0 ? t.windowBits = -t.windowBits : t.gzip && t.windowBits > 0 && t.windowBits < 16 && (t.windowBits += 16), this.err = 0, this.msg = ``, this.ended = !1, this.started = !1, this.chunks = [], this.result = new Uint8Array, this.strm = new Rn, this.strm.avail_out = 0;
			let n = bt(this.strm, t.level, 8, t.windowBits, t.memLevel, t.strategy, t.legacyHash);
			if (n !== 0) throw Error(P[n]);
			Wn.call(t.dictionary) === `[object ArrayBuffer]` && (t.dictionary = new Uint8Array(t.dictionary));
			let r = t.dictionary;
			if (r.length) {
				if (t.gzip) throw Error(`dictionary is not supported with gzip`);
				if (n = wt(this.strm, r), n !== 0) throw Error(P[n])
			}
		}
		push(e, t = !1) {
			let n = this.strm,
				r = this.options.chunkSize,
				i, a;
			if (this.ended) return !1;
			for (a = typeof t == `number` ? t : t === !0 ? 4 : 0, typeof e == `string` ? n.input = new TextEncoder().encode(e) : Wn.call(e) === `[object ArrayBuffer]` ? n.input = new Uint8Array(e) : n.input = e, n.next_in = 0, n.avail_in = n.input.length, this.started || (this.started = !0, this.onStart(n));;) {
				if (n.avail_out === 0 && (n.output = new Uint8Array(r), n.next_out = 0, n.avail_out = r), (a === 2 || a === 3) && n.avail_out <= 6) {
					this.onData(n.output.subarray(0, n.next_out)), n.avail_out = 0;
					continue
				}
				if (i = St(n, a), i === -2) break;
				if (i === 1) {
					n.next_out > 0 && this.onData(n.output.subarray(0, n.next_out)), i = Ct(this.strm);
					break
				}
				if (n.avail_out === 0) {
					this.onData(n.output);
					continue
				}
				if (a > 0 && n.next_out > 0) {
					this.onData(n.output.subarray(0, n.next_out)), n.avail_out = 0;
					continue
				}
				if (n.avail_in === 0) return !0
			}
			return this.err = i, this.msg = n.msg || P[i], this.ended = !0, this.onEnd(i), i === 0
		}
		onStart(e) {}
		onData(e) {
			this.chunks.push(e)
		}
		onEnd(e) {
			e === 0 && (this.result = Bn(this.chunks)), this.chunks = []
		}
	};

function qn(e, t = {}) {
	let n = new Kn(t);
	if (n.push(e, !0), n.err) throw Error(n.msg);
	return n.result
}

function Jn(e, t = {}) {
	return qn(e, Object.assign({}, t, {
		raw: !0
	}))
}

function Yn(e, t = {}) {
	return qn(e, Object.assign({}, t, {
		gzip: !0
	}))
}

function Xn(e, t) {
	var n = Object.keys(e);
	if (Object.getOwnPropertySymbols) {
		var r = Object.getOwnPropertySymbols(e);
		t && (r = r.filter(function(t) {
			return Object.getOwnPropertyDescriptor(e, t).enumerable
		})), n.push.apply(n, r)
	}
	return n
}

function Zn(e) {
	for (var t = 1; t < arguments.length; t++) {
		var n = arguments[t] == null ? {} : arguments[t];
		t % 2 ? Xn(Object(n), !0).forEach(function(t) {
			$(e, t, n[t])
		}) : Object.getOwnPropertyDescriptors ? Object.defineProperties(e, Object.getOwnPropertyDescriptors(n)) : Xn(Object(n)).forEach(function(t) {
			Object.defineProperty(e, t, Object.getOwnPropertyDescriptor(n, t))
		})
	}
	return e
}
var Qn = Object.prototype.toString,
	$n = {
		chunkSize: 1024 * 64,
		windowBits: 15,
		raw: !1,
		dictionary: new Uint8Array
	},
	er = class {
		constructor(e = {}) {
			$(this, `options`, void 0), $(this, `err`, void 0), $(this, `msg`, void 0), $(this, `ended`, void 0), $(this, `started`, void 0), $(this, `chunks`, void 0), $(this, `strm`, void 0), $(this, `result`, void 0), this.options = Object.assign({}, $n, e);
			let t = this.options;
			t.raw && t.windowBits >= 0 && t.windowBits < 16 && (t.windowBits = -t.windowBits, t.windowBits === 0 && (t.windowBits = -15)), t.windowBits >= 0 && t.windowBits < 16 && !e.windowBits && (t.windowBits += 32), t.windowBits > 15 && t.windowBits < 48 && (t.windowBits & 15 || (t.windowBits |= 15)), this.err = 0, this.msg = ``, this.ended = !1, this.started = !1, this.chunks = [], this.result = new Uint8Array, this.strm = new Rn, this.strm.avail_out = 0;
			let n = Dn(this.strm, t.windowBits);
			if (n !== 0) throw Error(P[n]);
			Qn.call(t.dictionary) === `[object ArrayBuffer]` && (t.dictionary = new Uint8Array(t.dictionary));
			let r = t.dictionary;
			if (t.raw && r.length && (n = Ln(this.strm, r), n !== 0)) throw Error(P[n])
		}
		push(e, t = !1) {
			let n = this.strm,
				r = this.options.chunkSize,
				i, a, o;
			if (this.ended) return this.err === 0;
			for (a = typeof t == `number` ? t : t === !0 ? 4 : 0, Qn.call(e) === `[object ArrayBuffer]` ? n.input = new Uint8Array(e) : n.input = e, n.next_in = 0, n.avail_in = n.input.length, this.started || (this.started = !0, this.onStart(n));;) {
				if (n.avail_out === 0 && (n.output = new Uint8Array(r), n.next_out = 0, n.avail_out = r), i = Pn(n, a), i === 2) {
					let e = this.options.dictionary;
					e.length && (i = Ln(n, e), i === 0 ? i = Pn(n, a) : i === -3 && (i = 2))
				}
				for (; n.avail_in > 0 && i === 1 && n.state.wrap & 2 && n.state.flags !== 0 && n.input[n.next_in] !== 0;) Tn(n), i = Pn(n, a);
				if (i === -2 || i === -3 || i === 2 || i === -4) break;
				if (o = n.avail_out, n.next_out && (n.avail_out === 0 || i === 1 || a > 0) && (this.onData(n.output.length === n.next_out ? n.output : n.output.subarray(0, n.next_out)), n.avail_out = 0, n.next_out = 0), !((i === 0 || i === -5) && o === 0)) {
					if (i === 1) {
						i = Fn(this.strm);
						break
					}
					if (n.avail_in === 0) {
						if (a === 4) {
							i = Fn(this.strm), i === 0 && (i = -5);
							break
						}
						return !0
					}
				}
			}
			return this.err = i, this.msg = n.msg || P[i], this.ended = !0, this.onEnd(i), i === 0
		}
		onStart(e) {}
		onData(e) {
			this.chunks.push(e)
		}
		onEnd(e) {
			e === 0 && (this.result = Bn(this.chunks)), this.chunks = []
		}
	};

function tr(e, t = {}) {
	let n = new er(t);
	if (n.push(e, !0), n.err) throw Error(n.msg);
	let r = n.result;
	return t.toText ? new TextDecoder().decode(r) : r
}

function nr(e, t = {}) {
	return tr(e, Zn(Zn({}, t), {}, {
		raw: !0
	}))
}
export {
	Kn as Deflate, zn as GZheader, er as Inflate, Rn as ZStream, Me as Z_BLOCK, Ve as Z_BUF_ERROR, ze as Z_DATA_ERROR, Le as Z_ERRNO, je as Z_FINISH, Ae as Z_FULL_FLUSH, Be as Z_MEM_ERROR, Ie as Z_NEED_DICT, De as Z_NO_FLUSH, Pe as Z_OK, Oe as Z_PARTIAL_FLUSH, Fe as Z_STREAM_END, Re as Z_STREAM_ERROR, ke as Z_SYNC_FLUSH, Ne as Z_TREES, qn as deflate, Jn as deflateRaw, Yn as gzip, tr as inflate, tr as ungzip, nr as inflateRaw, St as zlibDeflate, Ct as zlibDeflateEnd, xt as zlibDeflateInit, bt as zlibDeflateInit2, vt as zlibDeflateReset, _t as zlibDeflateResetKeep, wt as zlibDeflateSetDictionary, yt as zlibDeflateSetHeader, Pn as zlibInflate, Fn as zlibInflateEnd, In as zlibInflateGetHeader, On as zlibInflateInit, Dn as zlibInflateInit2, Tn as zlibInflateReset, En as zlibInflateReset2, wn as zlibInflateResetKeep, Ln as zlibInflateSetDictionary
};