from sys import *
f = open('icode.txt', 'r')
cmdf = open('kwtbl.h', 'w')
enumf = open('kwenum.h', 'w')
funf = open('funtbl.h', 'w')
strfunf = open('strfuntbl.h', 'w')
enumf.write('#ifndef __KWENUM_H\n#define __KWENUM_H\nenum token_t {\n')
funf.write('static const cmd_t funtbl[] BASIC_DAT = {\n')
count = 0
max_kw_len = 0
last_cmd = 0
nulls = []
strfuns = []
strfun_count = 0
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
  if fun != 'esyntax' and last_cmd > 0 and fun[0] != 's':
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
  if fun[0] == 's':
    if strfun_count == 0:
      strfun_first = count
    strfuns += [(enum, fun)]
    strfun_count += 1
    
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

strfunf.write('\nstatic const strfun_t strfuntbl[] BASIC_DAT = {\n')
for i in range(0, strfun_count):
  if i % 8 == 0:
    strfunf.write(' ')
  strfunf.write(' ' + strfuns[i][1] + ',')
  if i % 8 == 7:
    strfunf.write('\n')
strfunf.write('\n};\n')
strfunf.write('#define STRFUN_FIRST ' + str(strfun_first) + '\n')
strfunf.write('#define STRFUN_LAST ' + str(strfun_first + strfun_count) + '\n')
