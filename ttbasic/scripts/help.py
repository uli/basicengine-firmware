# This script creates helptext_${lang}.cpp, the file containing the content
# for the HELP command.
# first parameter: output file name
# second parameter: target language (default "en")
# third parameter: 'int' for interactive mode (default: non-interactive)

import re, os
from sys import stdin, stderr, argv
from trans import translate, init_trans, load_trans

import polib

out_file_name = argv[1]
of = open(out_file_name, 'w')

try:
    target_lang = argv[2]
except:
    target_lang = 'en'

input_po_name = '../po/helptext_' + target_lang + '.po'

if os.path.exists(input_po_name):
    pof = polib.pofile('../po/helptext_' + target_lang + '.po')
else:
    pof = polib.POFile()

init_trans(target_lang, pof)
if len(argv) > 3 and argv[3] == 'int':
    can_translate = load_trans()
else:
    can_translate = False

of.write('{\n"commands": [\n')

# expects input preprocessed with bdoc_1.sed
cmds = stdin.read().strip().strip('/***').strip('***/').split('***/\n/***')

def colorize_code(d):
    d = re.sub('`([^`]+)`', '\\\\\\\\Fn\g<1>\\\\\\\\Ff', d)
    d = re.sub('<<([^`]+)>>', '\\\\\\\\Fn\g<1>\\\\\\\\Ff', d)
    d = re.sub('(IMPORTANT:)', '\\\\\\\\Fp\g<1>\\\\\\\\Ff', d)
    d = re.sub('(WARNING:)', '\\\\\\\\Fp\g<1>\\\\\\\\Ff', d)
    d = re.sub('kbd:\[([^\]]*)]', '\\\\\\\\Fp[\g<1>]\\\\\\\\Ff', d)
    d = re.sub('\\\\endtable', '===', d)
    d = re.sub('\\\\table', '===', d)
    return d

def stringify_macros(d):
    # XXX: This results in strings like "4-1" instead of "3".
    return d

def gt(s):
    return '"' + s + '"'

def quote(s):
    escape = False
    ret = ''
    for i in s:
        if escape:
            ret += i
            escape = False
        elif i == '\\':
            escape = True
            ret += i
        elif i == '"':
            ret += '\\"'
        else:
            ret += i
    return ret

first_cmd = True

for c in sorted(cmds, key=lambda c: c.split('\n')[0].split(' ', 2)[2]):
    if first_cmd:
        first_cmd = False
    else:
        of.write(',\n')
    lines = c.split('\n')
    typ, section, token = lines[0].split(' ', 2)
    token = token.replace('\\', '')
    tokens = token.split(' ')
    tokens = [t for t in tokens if t not in ['(unary)', '(strings)']]

    item_data = {}
    current_item = "brief"
    item_data[current_item] = ""

    for l in lines[1:]:
        if l.startswith('\\') and not (l.startswith('\\table') or l.startswith('\\endtable')):
            # new item
            #print(l)
            sp = l.split(' ', 1)
            #print(sp)
            current_item = sp[0][1:]
            if current_item == 'sec':
              sp = [sp[1].lstrip()]
              current_item = sp[0].lower()
            # rest of the line can be content
            if len(sp) > 1:
                item_data[current_item] = sp[1].lstrip() + '\n'
            else:
                item_data[current_item] = ''
            continue

        item_data[current_item] += l + '\n'

    token_enums = []	# list of token enums
    for t in tokens:
        try:
            token_enums += [t]
        except:
            pass

    # emit command
    of.write('\n{\n  "command": "' + token + '",\n  "tokens": [ "' + '", "'.join(token_enums) + '" ],\n' +
          '  "section": "' + section + '",\n  "type": "' + typ + '"')

    # preprocess collected items
    for section, data in item_data.items():
        data = data.replace('\\]', ']')
        if section == 'args':
            data = re.sub(' +', ' ', data.replace(' +\n', '').replace('\n', ''))
            data = data.strip().replace('"', '\\"')[1:].split('@')
            try:
                data = ['\t'.join([d.split('\t', 1)[0], translate(d.split('\t', 1)[1].strip())]) for d in data]
            except IndexError:
                pass
            data = [colorize_code(d) for d in data]
            data = [stringify_macros(d) for d in data]
        elif section == 'ref':
            data = data.split(' ')
            data = [t.replace('_', ' ') for t in data]
            data = [t.replace('()', '') for t in data]
        else:
            # keep \table and \endtable on their own lines
            data = re.sub(r'\n(\\[a-z ]*)\n', '==LF==\g<1>==LF==', data)
            if section == 'usage':
                data = data.replace('==LF==', '\n').replace('"', '\\"')
            else:
                # translate all LFs that we want to keep to ==LF== and ditch the rest
                data = (data.replace('\n\n', '==LF==').	# paragraph breaks
                             replace('\n|', '==LF==|'). # table entries
                             replace('\n*', '==LF==*'). # itemized lists
                             replace('\n', ' ').	# drop all other LFs
                             replace('==LF==', '\n'))	# translate ==LF== back to real LFs
                data = re.sub(' +', ' ', data)		# drop asciidoc continuation character
                if not section in ['usage', 'ref']:
                    out=[]
                    for m in data.split('\n'):
                        m = m.strip()

                        # don't bother with formatting lines
                        if m == '' or m.startswith('\\') or '====' in m or '----' in m:
                          out += [m]
                          continue

                        # split at | to translate table columns separately
                        out += ['|'.join( [translate(mm) for mm in m.split('|')] )]
                    data = '\n'.join(out)
                        
                data = data.replace('"', '\\"')	# escape quotation marks
                data = colorize_code(data)
                data = stringify_macros(data)

        item_data[section] = data

    first_sec = False
    for e in item_data.keys():
        try:
            # if this succeeds, a regular plain-text item is available
            # convert to C string
            x = item_data[e].strip().replace('\n', '\\n').replace('\t', '\\t')
            if first_sec:
                first_sec = False
            else:
                of.write(',\n')
            of.write('  "' + e + '": ')
            if e == 'usage':
              of.write('"' + x + '"')
            else:
              of.write(gt(x))
        except KeyError:
            # item not available, print a suitable NULL entry
            pass
        except AttributeError:
            if first_sec:
                first_sec = False
            else:
                of.write(',\n')
            # list item ; must be ref or args
            if e == 'ref':
                # array of strings
                of.write('  "ref": [ ')
                of.write(', '.join(['"' + i.strip() + '"' for i in item_data[e]]))
                of.write(' ]')
            elif e == 'args':
                # array of arg_t
                of.write('  "args": {')
                first = True
                for i in item_data[e]:
                    if not first:
                        of.write(',')
                    else:
                        first = False
                    of.write('\n')
                    # unformat item
                    i = re.sub('\s+', ' ', i)
                    # split name and description
                    try:
                        name, descr = i.split(' ', 1)
                    except ValueError:
                        # no description
                        name = i
                        descr = ''
                    # unformat description (XXX: is that still necessary here?)
                    descr = descr.strip().replace('\n', ' ')
                    of.write('    "' + name.strip() + '": ' + gt(quote(descr)))
                of.write('\n  }')
            else:
                stderr.write('WARNING: unknown list object ' + e + '\n')
    of.write('\n}')
of.write('\n]\n}\n')
