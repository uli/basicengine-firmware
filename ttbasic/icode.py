from sys import *
f = open('icode.txt', 'r')
cmdf = open('kwtbl.h', 'w')
enumf = open('kwenum.h', 'w')
funf = open('funtbl.h', 'w')
strfunf = open('strfuntbl.h', 'w')
numfunf = open('numfuntbl.h', 'w')
enumf.write('#ifndef __KWENUM_H\n#define __KWENUM_H\nenum token_t {\n')
count = 0
cmds = []
strfuns = []
numfuns = []
for l in f.readlines():
  if l == '\n' or l.startswith('rem'):
    continue
  cmd, enum, fun = l.split()
  enumf.write(enum + ',')

  if fun[0] == 'i' or fun == 'ecom':
    cmds += [(cmd, enum, fun)]
  else:
    cmds += [(cmd, enum, 'NULL')]

  if fun[0] == 's':
    strfuns += [(enum, fun)]
  else:
    strfuns += [(enum, 'NULL')]

  if fun[0] == 'n':
    numfuns += [(enum, fun)]
  else:
    numfuns += [(enum, 'NULL')]

  count += 1

cmdf.write('\nstatic const char * const kwtbl_init[] = {\n')
for i in range(0, count):
  if i % 8 == 0:
    cmdf.write('  ')
  if cmds[i][0] == '_none':
    name = 'NULL'
  else:
    name = '"' + cmds[i][0] + '"'
  cmdf.write(name + ', ')
  if i % 8 == 7:
    cmdf.write('\n')
cmdf.write('\n};\n')
  
funf.write("#ifdef DECL_FUNCS\n")
dup=[]
for i in range(0, count):
  if not cmds[i][2] in dup and cmds[i][2] != 'NULL':
    funf.write('void ' + cmds[i][2] + '();\n')
    dup += [cmds[i][2]]

funf.write('#endif\n#ifdef DECL_TABLE\nconst Basic::cmd_t Basic::funtbl[] = {\n')

for i in range(0, count):
  if cmds[i][2] == 'NULL':
    handler = 'NULL'
  else:
    handler = '&Basic::' + cmds[i][2]
  funf.write(handler + ', ')

funf.write('\n};\n#endif\n')
enumf.write('\n};\n#endif\n')

strfunf.write("#ifdef DECL_FUNCS\n")
dup = []
for i in range(0, count):
  if not strfuns[i][1] in dup and strfuns[i][1] != 'NULL':
    strfunf.write("BString " + strfuns[i][1] + '();\n')
    dup += [strfuns[i][1]]

strfunf.write('\n#endif\n#ifdef DECL_TABLE\nconst Basic::strfun_t Basic::strfuntbl[] = {\n')
for i in range(0, count):
  if i % 8 == 0:
    strfunf.write(' ')
  if strfuns[i][1] == 'NULL':
    handler = 'NULL'
  else:
    handler = '&Basic::' + strfuns[i][1]
  strfunf.write(' ' + handler + ',')
  if i % 8 == 7:
    strfunf.write('\n')
strfunf.write('\n};\n#endif\n')

numfunf.write("#ifdef DECL_FUNCS\n")
dup=[]
for i in range(0, count):
  if not numfuns[i][1] in dup and numfuns[i][1] != 'NULL':
    numfunf.write("num_t " + numfuns[i][1] + '();\n')
    dup += [numfuns[i][1]]

numfunf.write('\n#endif\n#ifdef DECL_TABLE\nconst Basic::numfun_t Basic::numfuntbl[] = {\n')
for i in range(0, count):
  if i % 8 == 0:
    numfunf.write(' ')
  if numfuns[i][1] == 'NULL':
    handler = 'NULL'
  else:
    handler = '&Basic::' + numfuns[i][1]
  numfunf.write(' ' + handler + ',')
  if i % 8 == 7:
    numfunf.write('\n')
numfunf.write('\n};\n#endif\n')
