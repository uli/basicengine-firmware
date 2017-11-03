from sys import *
f = open('icode.txt', 'r')
cmdf = open('kwtbl.h', 'w')
enumf = open('kwenum.h', 'w')
funf = open('funtbl.h', 'w')
cmdf.write('static const char * const kwtbl[] __FLASH__ = {\n')
enumf.write('enum {\n')
funf.write('static const cmd_t funtbl[] __FLASH__ = {\n')
count = 0
max_kw_len = 0
for l in f.readlines():
  if len(l) == 0 or l.startswith('rem'):
    continue
  if count % 8 == 0:
    cmdf.write('  ')
    enumf.write('  ')
    funf.write('  ')
  cmd, enum, fun = l.split()
  if cmd != '_none':
    cmdf.write('"' + cmd + '",')
    if len(cmd) > max_kw_len:
      max_kw_len = len(cmd)
  enumf.write(enum + ',')
  funf.write(fun + ',')
  if count % 8 == 7:
    cmdf.write('\n')
    enumf.write('\n')
    funf.write('\n')
  else:
    cmdf.write(' ')
    enumf.write(' ')
    funf.write(' ')
  count += 1
cmdf.write('\n};\n')
cmdf.write('\n};\n\n#define MAX_KW_LEN ' + str(max_kw_len) + '\n')
enumf.write('\n};\n')
funf.write('\n};\n')
