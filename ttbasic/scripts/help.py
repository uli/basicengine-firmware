# This script creates helptext.cpp, the file containing the content for the
# HELP command.

import re
from sys import stdin, stderr

# create dict that maps token names to enums
tt = open("icode.txt", 'r').readlines()
toktrans = {}
for t in tt:
    try:
        name, enum, func = re.sub('\t+', '\t', t).split('\t')
    except ValueError:
        name, enum = re.sub('\t+', '\t', t).split('\t')
    if enum.startswith('X_'):		# extended token
        enum = '256 + ' + enum
    toktrans[name] = enum

print('#include "help.h"\n')
print('#pragma GCC diagnostic ignored "-Wmissing-field-initializers"\n')
print('#undef _\n')
print('#define _(s) (s)\n')
print('const struct help_t help[] = {')

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
    d = re.sub('{([^}]+)_m1}', '" XSTR(\g<1>-1) "', d)
    d = re.sub('{([^}]+)}', '" XSTR(\g<1>) "', d)
    return d

def gt(s):
    return '_("' + s + '")'

for c in cmds:
    lines = c.split('\n')
    typ, section, token = lines[0].split(' ', 2)
    token = token.replace('\\', '')
    tokens = token.split(' ')

    item_data = {}
    current_item = "brief"
    item_data[current_item] = ""

    for l in lines[1:]:
        if l.startswith('\\') and not (l.startswith('\\table') or l.startswith('\\endtable')):
            # new item
            current_item = l.split(' ')[0][1:]
            # rest of the line can be content
            item_data[current_item] = l.lstrip('\\abcdefghijklmnopqrstuvwxyz').lstrip() + '\n'
            continue

        item_data[current_item] += l + '\n'

    token_enums = []	# list of token enums
    for t in tokens:
        try:
            token_enums += [toktrans[t]]
        except:
            pass

    # emit command
    print('\n{\n  .command = "' + token + '",\n  .tokens = { ' + ', '.join(token_enums) + ', 0 },\n' +
          '  .section = SECTION_' + section.upper() + ',\n  .type = TYPE_' + typ.upper() + ',')

    # preprocess collected items
    for section, data in item_data.items():
        data = data.replace('\\]', ']')
        if section == 'args':
            data = data.strip().replace('"', '\\"')[1:].split('@')
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
                             replace('==LF==', '\n').	# translate ==LF== back to real LFs
                             replace('"', '\\"'))	# escape quotation marks
                data = re.sub(' +', ' ', data)		# drop asciidoc continuation character
                data = colorize_code(data)
                data = stringify_macros(data)

        item_data[section] = data

    for e in ['brief', 'usage', 'args', 'ret', 'desc', 'note', 'bugs', 'ref']:
        print('  .' + e + ' = ', end='')
        try:
            # if this succeeds, a regular plain-text item is available
            # convert to C string
            x = item_data[e].strip().replace('\n', '\\n"\n    "')
            if e == 'usage':
              print('"' + x + '",')
            else:
              print(gt(x) + ',')
        except KeyError:
            # item not available, print a suitable NULL entry
            if e in ['ref', 'args']:
                print('{ NULL, },')	# sentinel
            else:
                print('NULL,')
        except AttributeError:
            # list item ; must be ref or args
            if e == 'ref':
                # array of strings
                print('{', end='')
                for i in item_data[e]:
                    print(' "' + i.strip() + '",', end='')
                print(' NULL },')
            elif e == 'args':
                # array of arg_t
                print('{')
                for i in item_data[e]:
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
                    print('    { "' + name.strip() + '", ' + gt(descr) + ' },')
                print('    { NULL, NULL }\n  },')	# sentinel
            else:
                stderr.write('WARNING: unknown list object ' + e + '\n')
    print('},')
print('\n{},\n\n};\n')
print('#undef _\n')
