from sys import *
f = open('icode.txt', 'r')
cmdf = open('kwtbl.h', 'w')
enumf = open('kwenum.h', 'w')
funf = open('funtbl.h', 'w')
enumf.write('#ifndef __KWENUM_H\n#define __KWENUM_H\nenum token_t {\n')
funf.write('static const cmd_t funtbl[] GROUP(basic_data) = {\n')
count = 0
max_kw_len = 0
last_cmd = 0
nulls = []
for l in f.readlines():
  if len(l) == 0 or l.startswith('rem'):
    continue
  if count % 8 == 0:
    enumf.write('  ')
    funf.write('  ')
  cmd, enum, fun = l.split()

  if cmd == '_none':
    nulls += [count]
  else:
    cmdf.write('static const char __cmd' + str(count) + ' [] PROGMEM = "' + cmd + '";\n')
  if len(cmd) > max_kw_len:
    max_kw_len = len(cmd)
    
  enumf.write(enum + ',')
  if fun != 'esyntax' and last_cmd > 0:
    print 'esyntax tokens must be last'
    exit(1)
  if fun == 'esyntax' and last_cmd == 0:
    last_cmd = count
  if last_cmd == 0:
    funf.write(fun + ',')
  if count % 8 == 7:
    enumf.write('\n')
    if last_cmd == 0:
      funf.write('\n')
  else:
    enumf.write(' ')
    if last_cmd == 0:
      funf.write(' ')
  count += 1

cmdf.write('\nstatic const char * const kwtbl[] PROGMEM = {\n')
for i in range(0, count):
  if i % 8 == 0:
    cmdf.write('  ')
  if i in nulls:
    cmdf.write('NULL, ')
  else:
    cmdf.write('__cmd' + str(i) + ', ')
  if i % 8 == 7:
    cmdf.write('\n')
  
cmdf.write('\n};\n\n#define MAX_KW_LEN ' + str(max_kw_len) + '\n')
enumf.write('\n};\n#endif\n')
funf.write('\n};\n')
