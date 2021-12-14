import sys

if sys.version_info.major < 3:
    print('This is a python3 script')
    sys.exit(-1)

import struct
from math import isnan, isfinite

if len(sys.argv) != 2:
    print('Usage: python3 compare_shdfil.py MunkB_Coh')
    print('No path, no .shd')
    sys.exit(1)

thresh_ulp_print = 4000
thresh_ulp_alarm = 100000
thresh_abs_print = 1e-7
thresh_abs_alarm = 1e-5
thresh_rel_print = 1e-4
thresh_rel_alarm = 1e-2

cxxfile = 'temp/cxx/{}.shd'.format(sys.argv[1])
forfile = 'temp/FORTRAN/{}.shd'.format(sys.argv[1])

with open(cxxfile, 'rb') as cxxf, open(forfile, 'rb') as forf:
    cxxdata = cxxf.read()
    fordata = forf.read()
    if len(cxxdata) != len(fordata):
        print('File lengths differ: {} vs {}'.format(len(cxxdata), len(fordata)))
        sys.exit(0)
    
    reclen = 0
    def read_rec_int(data, rec, val):
        start = 4*(rec*reclen+val)
        return struct.unpack('I', data[start:start+4])[0]
    cxxreclen = read_rec_int(cxxdata, 0, 0)
    forreclen = read_rec_int(fordata, 0, 0)
    if cxxreclen != forreclen:
        print('Record lengths differ: {} vs {}'.format(cxxreclen, forreclen))
        sys.exit(0)
    reclen = cxxreclen
    
    for w in range(10*reclen):
        cxxd = cxxdata[w*4:(w+1)*4]
        ford = fordata[w*4:(w+1)*4]
        if cxxd != ford:
            print('{:08X}: CXX {:08X}  FOR {:08X}'.format(w*4,
                struct.unpack('I', cxxd)[0], struct.unpack('I', ford)[0]))
    
    NSz = read_rec_int(fordata, 2, 4)
    NRz = read_rec_int(fordata, 2, 5)
    NRr = read_rec_int(fordata, 2, 6)
    assert NSz == read_rec_int(cxxdata, 2, 4)
    assert NRz == read_rec_int(cxxdata, 2, 5)
    assert NRr == read_rec_int(cxxdata, 2, 6)
    assert NRr * 2 <= reclen
    if len(cxxdata) != NSz * NRz * reclen * 4 + 4 * 10 * reclen:
        print('NSz {} NRz {} NRr {} reclen {}'.format(NSz, NRz, NRr, reclen))
        raise RuntimeError('Invalid file size')
    
    errored = False
    for addr in range(4*10*reclen, len(cxxdata), 8):
        cxxd = cxxdata[addr:addr+8]
        ford = fordata[addr:addr+8]
        if cxxd == ford:
            continue
        cir, cii = struct.unpack('II', cxxd)
        fir, fii = struct.unpack('II', ford)
        cfr, cfi = struct.unpack('ff', cxxd)
        ffr, ffi = struct.unpack('ff', ford)
        if isnan(cfr) != isnan(ffr) or isnan(cfi) != isnan(ffi):
            ulpr = ulpi = absr = absi = relr = reli = 10000000000
        else:
            ulpr, ulpi = abs(cir - fir), abs(cii - fii)
            absr, absi = abs(cfr - ffr), abs(cfi - ffi)
            relr = absr / abs(ffr) if ffr != 0 else absr
            reli = absi / abs(ffi) if ffi != 0 else absi
        # If the absolute error is miniscule, it doesn't matter what the relative
        # or ULP error are
        if absr < 1e-10:
            ulpr = 0
            relr = 0.0
        if absi < 1e-10:
            ulpi = 0
            reli = 0.0
        daddr = (addr - 4*10*reclen) // 8
        s = daddr // (NRz * (reclen // 2))
        Rz = (daddr // (reclen // 2)) % NRz
        Rr = daddr % (reclen // 2)
        p_Rr = ('INVALID ' if Rr >= NRr else '') + '{:3}'.format(Rr)
        p_addr = 'src {:3} iz {:3} ir {} {:08X}: '.format(s, Rz, p_Rr, addr)
        p_int = '{:08X} {:08X} / {:08X} {:08X} | '.format(cir, cii, fir, fii)
        p_ulp = 'ULP ({:4},{:4}) | '.format(ulpr, ulpi)
        fmt_flt = '{:12.6}'
        fmt_flt2 = '({},{}) / ({},{}) | '.format(fmt_flt, fmt_flt, fmt_flt, fmt_flt)
        p_flt = fmt_flt2.format(cfr, cfi, ffr, ffi)
        p_abs = 'ABS ({},{}) | '.format(fmt_flt, fmt_flt).format(absr, absi)
        p_rel = 'REL ({:9.6}%,{:9.6}%) | '.format(relr*100.0, reli*100.0)
        if ulpr > thresh_ulp_alarm or ulpi > thresh_ulp_alarm or \
            absr > thresh_abs_alarm or absi > thresh_abs_alarm or \
            relr > thresh_rel_alarm or reli > thresh_rel_alarm:
            if not errored:
                print('\nERROR: Extremely large error(s) detected:')
                errored = True
            else:
                print(p_addr + p_int + p_flt + p_ulp + p_abs + p_rel)
        if errored:
            continue
        errs = ''
        show_int, show_flt = False, False
        if ulpr > thresh_ulp_print or ulpi > thresh_ulp_print:
            errs += p_ulp
            show_int = True
        if absr > thresh_abs_print or absi > thresh_abs_print:
            errs += p_abs
            show_flt = True
        if relr > thresh_rel_print or reli > thresh_rel_print:
            errs += p_rel
            show_flt = True
        if not show_int and not show_flt:
            continue
        if show_flt:
            errs = p_flt + errs
        if show_int:
            errs = p_int + errs
        print(p_addr + errs)

    if errored:
        sys.exit(1)