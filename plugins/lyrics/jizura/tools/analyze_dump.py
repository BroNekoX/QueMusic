#!/usr/bin/env python3
"""Turn a Windows minidump of a Qt crash into a readable stack.

    python tools/analyze_dump.py "%LOCALAPPDATA%\\CrashDumps\\QueMusic.exe.1234.dmp"

Why this exists: a crash inside V4 (Qt's JS engine) usually reports a location
in unrelated code - `QV4::Value::sameValueZero`, the memory manager - because the
actual corruption happened earlier.  WER only gives you a module and an offset,
which says nothing about who called it.  This walks the faulting thread's stack
and names every frame it can, using the .pdata function table and the export
table of each loaded DLL, so the real call chain is visible without a debugger
or private symbols.

Limitations, stated up front:
  * It scans the raw stack for pointers into executable sections (there are no
    frame pointers in release builds), so it can miss frames and occasionally
    show a stale pointer.  The first few entries below the faulting frame are
    the reliable ones.
  * Only the faulting thread's stack is walked.
  * Names come from exports; Qt exports a lot, but a private function may show
    as the nearest exported symbol instead of its own name.
"""
import os
import struct
import sys

MDMP = b"MDMP"
STREAM_THREAD_LIST = 3
STREAM_MODULE_LIST = 4
STREAM_MEMORY_LIST = 5
STREAM_EXCEPTION = 6
STREAM_MEMORY64_LIST = 9

# x64 CONTEXT offsets
CTX_RSP = 0x98
CTX_RIP = 0xF8


class Dump:
    def __init__(self, path):
        with open(path, "rb") as f:
            self.data = f.read()
        d = self.data
        if d[:4] != MDMP:
            raise ValueError("not a minidump")
        nstreams, dir_rva = struct.unpack_from("<II", d, 8)
        self.streams = {}
        for i in range(nstreams):
            t, size, rva = struct.unpack_from("<III", d, dir_rva + i * 12)
            self.streams[t] = (size, rva)
        self.modules = self._modules()
        self.ranges = self._memory_ranges()

    def _modules(self):
        out = []
        if STREAM_MODULE_LIST not in self.streams:
            return out
        _, rva = self.streams[STREAM_MODULE_LIST]
        n = struct.unpack_from("<I", self.data, rva)[0]
        for i in range(n):
            off = rva + 4 + i * 108
            base, size = struct.unpack_from("<QI", self.data, off)
            name_rva = struct.unpack_from("<I", self.data, off + 20)[0]
            ln = struct.unpack_from("<I", self.data, name_rva)[0]
            name = self.data[name_rva + 4:name_rva + 4 + ln].decode("utf-16-le", "replace")
            out.append({"base": base, "size": size, "name": name})
        return out

    def _memory_ranges(self):
        out = []
        if STREAM_MEMORY64_LIST in self.streams:
            _, rva = self.streams[STREAM_MEMORY64_LIST]
            n, base_rva = struct.unpack_from("<QQ", self.data, rva)
            cur = base_rva
            for i in range(n):
                start, size = struct.unpack_from("<QQ", self.data, rva + 16 + i * 16)
                out.append((start, size, cur))
                cur += size
        elif STREAM_MEMORY_LIST in self.streams:
            _, rva = self.streams[STREAM_MEMORY_LIST]
            n = struct.unpack_from("<I", self.data, rva)[0]
            for i in range(n):
                start, dsize, drva = struct.unpack_from("<QII", self.data, rva + 4 + i * 16)
                out.append((start, dsize, drva))
        out.sort()
        return out

    def read(self, addr, size):
        for start, length, rva in self.ranges:
            if start <= addr < start + length:
                n = min(size, start + length - addr)
                off = rva + (addr - start)
                return self.data[off:off + n]
        return None

    def threads(self):
        """MINIDUMP_THREAD[..]: stack range and context per thread."""
        out = []
        if STREAM_THREAD_LIST not in self.streams:
            return out
        _, rva = self.streams[STREAM_THREAD_LIST]
        n = struct.unpack_from("<I", self.data, rva)[0]
        for i in range(n):
            off = rva + 4 + i * 48
            tid = struct.unpack_from("<I", self.data, off)[0]
            stack_start, stack_size, stack_rva = struct.unpack_from("<QII", self.data, off + 24)
            ctx_size, ctx_rva = struct.unpack_from("<II", self.data, off + 40)
            out.append({"tid": tid, "stack": (stack_start, stack_size, stack_rva),
                        "ctx_size": ctx_size, "ctx_rva": ctx_rva})
        return out

    def exception(self):
        if STREAM_EXCEPTION not in self.streams:
            return None
        _, rva = self.streams[STREAM_EXCEPTION]
        # MINIDUMP_EXCEPTION_STREAM: ThreadId(4) align(4) MINIDUMP_EXCEPTION(152)
        # then the ThreadContext location descriptor -> context at +160
        tid = struct.unpack_from("<I", self.data, rva)[0]
        code = struct.unpack_from("<I", self.data, rva + 8)[0]
        exc_addr = struct.unpack_from("<Q", self.data, rva + 24)[0]
        ctx_size, ctx_rva = struct.unpack_from("<II", self.data, rva + 160)
        res = {"tid": tid, "code": code, "address": exc_addr, "rip": None, "rsp": None, "stack": None}

        # the faulting thread's own record also carries a context and its stack
        for t in self.threads():
            if t["tid"] == tid:
                res["stack"] = self._read(t["stack"][0], t["stack"][1]) if t["stack"][1] else None
                if not ctx_size and t["ctx_size"]:
                    ctx_size, ctx_rva = t["ctx_size"], t["ctx_rva"]
                break
        if ctx_size >= CTX_RIP + 8:
            ctx = self.data[ctx_rva:ctx_rva + ctx_size]
            res["rip"] = struct.unpack_from("<Q", ctx, CTX_RIP)[0]
            res["rsp"] = struct.unpack_from("<Q", ctx, CTX_RSP)[0]
        else:
            res["rip"] = exc_addr
        return res

    def _read(self, start, size):
        for s, length, rva in self.ranges:
            if s <= start < s + length:
                n = min(size, s + length - start)
                off = rva + (start - s)
                return self.data[off:off + n]
        return None

    def module_of(self, addr):
        for m in self.modules:
            if m["base"] <= addr < m["base"] + m["size"]:
                return m
        return None


class Symbols:
    """Nearest exported symbol + containing function from .pdata, per DLL."""

    def __init__(self):
        self.cache = {}
        self.exports = {}
        self.funcs = {}

    def load(self, path):
        if path in self.cache:
            return self.cache[path]
        info = {"exports": [], "funcs": []}
        try:
            d = open(path, "rb").read()
        except OSError:
            self.cache[path] = info
            return info
        try:
            pe = struct.unpack_from("<I", d, 0x3C)[0]
            nsec = struct.unpack_from("<H", d, pe + 6)[0]
            optsz = struct.unpack_from("<H", d, pe + 20)[0]
            opt = pe + 24
            if struct.unpack_from("<H", d, opt)[0] != 0x20B:
                raise ValueError("not PE32+")
            nrva = struct.unpack_from("<I", d, opt + 108)[0]
            rva_off = opt + 112
            secs = []
            for i in range(nsec):
                b = opt + optsz + i * 40
                name = d[b:b + 8].rstrip(b"\0").decode("ascii", "replace")
                vsize, vaddr, rsize, raw = struct.unpack_from("<IIII", d, b + 8)
                secs.append((name, vaddr, vsize, raw, rsize))

            def r2o(rva):
                for name, vaddr, vsize, raw, rsize in secs:
                    if vaddr <= rva < vaddr + max(vsize, rsize):
                        return raw + (rva - vaddr)
                return None

            for name, vaddr, vsize, raw, rsize in secs:
                if name == ".pdata":
                    for off in range(0, vsize, 12):
                        s, e, u = struct.unpack_from("<III", d, raw + off)
                        if s:
                            info["funcs"].append((s, e))
            info["funcs"].sort()

            exp_rva, _ = struct.unpack_from("<II", d, rva_off)
            if exp_rva:
                eo = r2o(exp_rva)
                nfun, nnam = struct.unpack_from("<II", d, eo + 20)
                afun, anam, aord = struct.unpack_from("<III", d, eo + 28)
                fo, no, oo = r2o(afun), r2o(anam), r2o(aord)
                for i in range(nnam):
                    nr = struct.unpack_from("<I", d, no + 4 * i)[0]
                    sr = r2o(nr)
                    if sr is None:
                        continue
                    end = d.index(b"\0", sr)
                    nm = d[sr:end].decode("ascii", "replace")
                    ordi = struct.unpack_from("<H", d, oo + 2 * i)[0]
                    fr = struct.unpack_from("<I", d, fo + 4 * ordi)[0]
                    info["exports"].append((fr, nm))
            info["exports"].sort()
        except Exception:
            pass
        self.cache[path] = info
        return info

    @staticmethod
    def _floor(lst, rva):
        lo, hi, best = 0, len(lst) - 1, None
        while lo <= hi:
            m = (lo + hi) // 2
            if lst[m][0] <= rva:
                best = lst[m]
                lo = m + 1
            else:
                hi = m - 1
        return best

    def describe(self, path, rva):
        info = self.load(path)
        f = self._floor(info["funcs"], rva)
        e = self._floor(info["exports"], rva)
        fn = ""
        if f and f[0] <= rva < f[1]:
            fn = "func@0x%x+%d" % (f[0], rva - f[0])
        name = ""
        if e:
            name = e[1]
            if f and f[0] <= rva < f[1] and e[0] < f[0]:
                name += " ..."
        if name and fn:
            return "%s   [%s]" % (demangle(name), fn)
        return demangle(name) if name else (fn or "(unknown)")

    def address(self, dump, addr):
        m = dump.module_of(addr)
        if not m:
            return None
        return os.path.basename(m["name"]), addr - m["base"], m["name"]


def demangle(n):
    """Make Itanium-mangled Qt symbols readable enough."""
    if not n.startswith("_Z"):
        return n
    s = n[2:]
    if s.startswith("N") and "E" in s:
        body = s[1:s.index("E")]
        parts, i = [], 0
        while i < len(body):
            j = i
            while j < len(body) and body[j].isdigit():
                j += 1
            if j == i:
                i += 1
                continue
            ln = int(body[i:j])
            parts.append(body[j:j + ln])
            i = j + ln
        return "::".join(parts)
    return n


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    path = sys.argv[1]
    dump = Dump(path)
    ex = dump.exception()
    if not ex:
        print("no exception stream")
        return 1
    syms = Symbols()
    print("dump      : %s" % path)
    print("exception : 0x%08X  (%s)" % (ex["code"],
          {0xC0000005: "access violation", 0xC00000FD: "stack overflow",
           0xC0000374: "heap corruption"}.get(ex["code"], "?")))
    at = syms.address(dump, ex["rip"])
    if at:
        print("faulting  : %s + 0x%x\n            %s" % (at[0], at[1], syms.describe(at[2], at[1])))
    else:
        print("faulting  : 0x%x (not in a known module)" % ex["rip"])

    print("\nstack (nearest frames first):")
    seen = 0
    stack = None
    if ex["rsp"]:
        stack = dump.read(ex["rsp"], 96 * 1024)
    if not stack:
        stack = ex.get("stack")
    if not stack:
        print("  could not read the stack")
        return 1
    prev = None
    for i in range(0, len(stack) - 8, 8):
        v = struct.unpack_from("<Q", stack, i)[0]
        a = syms.address(dump, v)
        if not a:
            continue
        mod, rva, full = a
        if v == prev:
            continue
        prev = v
        # only frames that are actually inside a function
        desc = syms.describe(full, rva)
        print("  [%02d] %-16s +0x%-8x %s" % (i // 8, mod, rva, desc))
        seen += 1
        if seen >= 30:
            break
    return 0


if __name__ == "__main__":
    sys.exit(main())
