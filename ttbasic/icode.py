from sys import *
f = open('icode.txt', 'r')
cmdf = open('kwtbl.h', 'w')
enumf = open('kwenum.h', 'w')
funf = open('funtbl.h', 'w')
strfunf = open('strfuntbl.h', 'w')
numfunf = open('numfuntbl.h', 'w')
enumf.write('#ifndef __KWENUM_H\n#define __KWENUM_H\nenum token_t {\n')
count = 0
last_cmd = 0
nulls = []
cmds = []
cmds_count = 0
strfuns = []
strfun_count = 0
numfuns = []
numfun_count = 0
extended = []
ext_count = 0
for l in f.readlines():
  if len(l) == 0 or l.startswith('rem'):
    continue
  cmd, enum, fun = l.split()
  if not enum.startswith('X_'):
    if cmd == '_none':
      nulls += [count]
    else:
      cmdf.write('static const char __cmd' + str(count) + ' [] PROGMEM = "' + cmd + '";\n')

    enumf.write(enum + ',')

    if fun != 'esyntax' and last_cmd > 0 and fun[0] != 's' and fun[0] != 'n':
      print 'esyntax tokens must be last'
      exit(1)
    if fun == 'esyntax' and last_cmd == 0:
      last_cmd = count
    if last_cmd == 0:
      cmds += [(cmd, enum, fun)]
      cmds_count += 1
    if fun[0] == 's':
      if strfun_count == 0:
        strfun_first = count
      strfuns += [(enum, fun)]
      strfun_count += 1
    elif fun[0] == 'n':
      if numfun_count == 0:
        numfun_first = count
      numfuns += [(enum, fun)]
      numfun_count += 1
    count += 1
  else:	# X_
    if ext_count == 0:
      ext_first = count
    extended += [(cmd, enum, fun)]
    ext_count += 1


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
cmdf.write('\n};\n')
  
max_kw_ext_len = 0

for i in range(0, cmds_count):
  funf.write('void ' + cmds[i][2] + '();\n')

funf.write('static const cmd_t funtbl[] BASIC_DAT = {\n')

for i in range(0, cmds_count):
  max_kw_ext_len = max(max_kw_ext_len, len(cmds[i][0]))
  funf.write(cmds[i][2] + ', ')

funf.write('\n};\n')

enumf.write('\n};\n')
enumf.write('enum token_ext_t {\n')

for i in range(0, ext_count):
  funf.write('void ' + extended[i][2] + '();\n')

funf.write('static const cmd_t funtbl_ext[] BASIC_DAT = {\n')

for i in range(0, ext_count):
  cmdf.write('static const char __cmd_ext' + str(i) + ' [] PROGMEM = "' + extended[i][0] + '";\n')
  max_kw_ext_len = max(max_kw_ext_len, len(extended[i][0]))
  enumf.write(extended[i][1] + ', ')
  funf.write(extended[i][2] + ', ')

cmdf.write('\nstatic const char * const kwtbl_ext[] PROGMEM = {\n')
for i in range(0, ext_count):
  if i % 8 == 0:
    cmdf.write('  ')
  cmdf.write('__cmd_ext' + str(i) + ', ')
  if i % 8 == 7:
    cmdf.write('\n')
  
cmdf.write('\n};\n')
enumf.write('\n};\n#endif\n')
funf.write('\n};\n')

for i in range(0, strfun_count):
  strfunf.write("BString " + strfuns[i][1] + '();\n')
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

for i in range(0, numfun_count):
  numfunf.write("num_t " + numfuns[i][1] + '();\n')
numfunf.write('\nstatic const numfun_t numfuntbl[] BASIC_DAT = {\n')
for i in range(0, numfun_count):
  if i % 8 == 0:
    numfunf.write(' ')
  numfunf.write(' ' + numfuns[i][1] + ',')
  if i % 8 == 7:
    numfunf.write('\n')
numfunf.write('\n};\n')
numfunf.write('#define NUMFUN_FIRST ' + str(numfun_first) + '\n')
numfunf.write('#define NUMFUN_LAST ' + str(numfun_first + numfun_count) + '\n')
