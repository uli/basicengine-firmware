from sys import *
f = open('icode.txt', 'r')
cmdf = open('kwtbl.h', 'w')
enumf = open('kwenum.h', 'w')
funf = open('funtbl.h', 'w')
enumf.write('#ifndef __KWENUM_H\n#define __KWENUM_H\nenum token_t {\n')
funf.write('static const cmd_t funtbl[] __FLASH__ = {\n')
count = 0
max_kw_len = 0
last_cmd = 0
for l in f.readlines():
  if len(l) == 0 or l.startswith('rem'):
    continue
  if count % 8 == 0:
    enumf.write('  ')
    funf.write('  ')
  cmd, enum, fun = l.split()
  if cmd != '_none':
    if last_cmd > 0:
      print 'error: _none commands must be last'
      exit(1)
    cmdf.write('static const char __cmd' + str(count) + ' [] PROGMEM = "' + cmd + '";\n')
    if len(cmd) > max_kw_len:
      max_kw_len = len(cmd)
  else:
    if last_cmd == 0:
      last_cmd = count
    
  enumf.write(enum + ',')
  funf.write(fun + ',')
  if count % 8 == 7:
    enumf.write('\n')
    funf.write('\n')
  else:
    enumf.write(' ')
    funf.write(' ')
  count += 1

if last_cmd == 0:
  last_cmd = count

cmdf.write('\nstatic const char * const kwtbl[] PROGMEM = {\n')
for i in range(0, last_cmd):
  if i % 8 == 0:
    cmdf.write('  ')
  cmdf.write('__cmd' + str(i) + ', ')
  if i % 8 == 7:
    cmdf.write('\n')
  
cmdf.write('\n};\n\n#define MAX_KW_LEN ' + str(max_kw_len) + '\n')
enumf.write('\n};\n#endif\n')
funf.write('\n};\n')
