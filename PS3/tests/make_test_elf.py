#!/usr/bin/env python3
"""Builds a tiny PPC64 big-endian ELF (PS3-style OPD entry) used to smoke-test the pipeline.
Expected output when run through ps3core: "OK"."""
import struct, sys

def li(rt, imm):   return (14<<26)|(rt<<21)|(imm & 0xFFFF)
def lis(rt, imm):  return (15<<26)|(rt<<21)|(imm & 0xFFFF)
def addi(rt, ra, imm): return (14<<26)|(rt<<21)|(ra<<16)|(imm & 0xFFFF)
def ori(ra, rs, imm):  return (24<<26)|(rs<<21)|(ra<<16)|(imm & 0xFFFF)
def mtspr(spr, rs): return (31<<26)|(rs<<21)|((spr&31)<<16)|(((spr>>5)&31)<<11)|(467<<1)
def mfspr(rt, spr): return (31<<26)|(rt<<21)|((spr&31)<<16)|(((spr>>5)&31)<<11)|(339<<1)
def cmpwi(ra, imm): return (11<<26)|(0<<23)|(0<<21)|(ra<<16)|(imm & 0xFFFF)
def bc(bo, bi, off): return (16<<26)|(bo<<21)|(bi<<16)|(off & 0xFFFC)
def b(off, lk=0): return (18<<26)|(off & 0x03FFFFFC)|lk
BLR = 0x4E800020
SC = 0x44000002

TEXT, DATA = 0x10000, 0x20000
prog = []   # (label or None, word-or-lambda)
labels = {}
def L(name): labels[name] = TEXT + 4*len(prog)
def emit(w): prog.append(w)

emit(li(6, 0)); emit(li(7, 10)); emit(mtspr(9, 7))           # r6=0; ctr=10
L('loop'); emit(addi(6, 6, 1))
emit(lambda pc: bc(16, 0, labels['loop'] - pc))                # bdnz loop
emit(lambda pc: b(labels['func'] - pc, 1))                     # bl func
emit(cmpwi(6, 10))
emit(lambda pc: bc(4, 2, labels['fail'] - pc))                 # bne fail
emit(li(11, 403)); emit(li(3, 0)); emit(lis(4, 2)); emit(li(5, 3)); emit(li(6, 0)); emit(SC)
emit(lambda pc: b(labels['exit'] - pc))
L('fail'); emit(li(11, 403)); emit(li(3, 0)); emit(lis(4, 2)); emit(ori(4, 4, 0x10)); emit(li(5, 5)); emit(li(6, 0)); emit(SC)
L('exit'); emit(li(11, 22)); emit(SC)
L('func'); emit(mfspr(0, 8)); emit(mtspr(8, 0)); emit(BLR)

text = b''
for i, w in enumerate(prog):
    pc = TEXT + 4*i
    text += struct.pack('>I', w(pc) if callable(w) else w)

data = bytearray(0x40)
data[0:3] = b'OK\n'; data[0x10:0x15] = b'FAIL\n'
struct.pack_into('>QQ', data, 0x20, TEXT, 0x30000)           # OPD {entry, toc}
entry = DATA + 0x20

phoff, off_text, off_data = 64, 64 + 2*56, 64 + 2*56 + len(text)
ehdr = b'\x7fELF' + bytes([2, 2, 1, 0]) + bytes(8) + struct.pack('>HHIQQQIHHHHHH', 2, 21, 1, entry, phoff, 0, 0, 64, 56, 2, 0, 0, 0)
ph  = struct.pack('>IIQQQQQQ', 1, 5, off_text, TEXT, TEXT, len(text), len(text), 0x10000)
ph += struct.pack('>IIQQQQQQ', 1, 6, off_data, DATA, DATA, len(data), len(data), 0x10000)
open(sys.argv[1] if len(sys.argv) > 1 else 'test.elf', 'wb').write(ehdr + ph + text + bytes(data))
