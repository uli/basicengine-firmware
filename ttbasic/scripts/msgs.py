# creates system message header files from gettext message catalogs (po
# files) and/or errdef.h
# first parameter is output file name, second parameter is target language
# if there is a third parameter and it is 'int' it will use machine
# translation and interactive review

from sys import stdin, stdout, stderr, argv
import polib
from trans import init_trans, load_trans, translate

out_file_name = argv[1]
target_lang = argv[2]

pofile_name = '../po/system_' + target_lang + '.po'

try:
  pof = polib.pofile(pofile_name)
except:
  pof = polib.POFile()

init_trans(target_lang, pof)
if len(argv) > 3 and argv[3] == 'int':
    load_trans()

of = open(out_file_name, 'w')

of.write('const struct msg msgs_' + target_lang + '[] = {\n')

cont = False
for l in stdin.readlines():
    if cont == True:
        if l.startswith('"'):
            msg += l.split('"')[1]
            continue
        else:
            cont = False
            if len(msg) == 0: continue
    else:
        if not (l.startswith('msgid') or l.startswith('ESTR(')):
            continue

        if l.startswith('ESTR('):
            msg = l.split('"')[1]
        else:
            msg = l.split(' ', 1)[1][1:-2]

        if len(msg) == 0:
            cont = True
            continue

    # massage to make it easier for the translator
    tmsg = msg
    tmsg = tmsg.replace('\\n', '\n')
    if tmsg.startswith('expected '):
      tmsg = tmsg.replace('expected ','expected: ')

    tr = translate(tmsg)

    tr = tr.replace('\n', '\\n')
    of.write('  { "' + msg + '",')
    if len(msg) > 80:
      of.write('\n   ')
    of.write(' "' + tr + '" },\n')

of.write('  { NULL, NULL },\n')
of.write('};\n')
