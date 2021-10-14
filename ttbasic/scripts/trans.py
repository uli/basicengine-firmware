# Pipes a message through Google Translate and Argos Translate, then
# translates the results back to German to check and presents the
# user with a choice.

from sys import stdin, stderr, argv
import getpass
import polib

target_lang = 'en'
retrans_lang = 'de'
can_translate = False

def init_trans(tl, p):
    global target_lang, pof, bakpof, bulkpof
    target_lang = tl
    pof = p
    try:
      bulkpof = polib.pofile('../bulk_' + tl + '.po')
    except:
      bulkpof = None
    bakpof = open('/tmp/bakpof.txt', 'w')

def load_trans():
    global target_lang, retrans_lang, can_translate, latr, backtr, trans

    if target_lang == 'de':
      retrans_lang = 'en'
    else:
      retrans_lang = 'de'

    try:
        from googletrans import Translator
        from argostranslate import package, translate

        trans = Translator()
        il = translate.get_installed_languages()
        for en in il:
          if en.code == 'en':
            break
        for tlang in il:
          if tlang.code == target_lang:
            break
        latr = en.get_translation(tlang)
        backtr = tlang.get_translation(en)
        can_translate = True
        return True
    except Exception as e:
        stderr.write('WARNING: could not load translation libraries: ' + str(e) + '\n')
        return False

def translate_g(tmsg):
    try:
      tr = trans.translate(tmsg, src='en', dest=target_lang).text

      try:
        btr = backtr.translate(tr)
      except:
        btr = '*failed*'

      try:
        bbtr = trans.translate(tr,src=target_lang,dest=retrans_lang).text
      except:
        bbtr = '*failed*'

      return tr, btr, bbtr
    except (AttributeError, TypeError):
      return tmsg,'google fail','google fail'

def translate_a(tmsg):
    try:
      tr2 = latr.translate(tmsg)

      try:
        btr2 = backtr.translate(tr2)
      except:
        btr2 = '*failed*'

      try:
        bbtr2 = trans.translate(tr2,src=target_lang,dest=retrans_lang).text
      except:
        bbtr2 = '*failed*'

      return tr2,btr2,bbtr2
    except (AttributeError, TypeError):
      return tmsg,'argos fail','argos fail'

W='\033[0m'
R='\033[31m'
G='\033[32m'
O='\033[33m'
B='\033[34m'

def translate(m):
    if target_lang == 'en' or len(m) == 0 or len([c for c in m if c.isalpha()]) == 0:
        return m

    try:
        e = pof.find(m)
        #stderr.write('found ' + e.msgid + ' as ' + e.msgstr + '\n')
        return e.msgstr
    except AttributeError:
        pass

    # Google Translate completely freaks out and corrupts the formatting
    # when encountering the string "<<". Your guess is as good as mine.
    if '<<' not in m:
      bakpof.write(m + '\n\n=====\n\n')

    if can_translate == False:
        return m

    stderr.write('\n'+R+'==='+W+' SOURCE\n')
    stderr.write(m +'\n')
    tm,btr,bbtr = translate_a(m)
    stderr.write(G+'+++'+W+' Argos\n')
    stderr.write(tm +'\n')
    stderr.write('['+btr +']\n')
    stderr.write('['+bbtr +']\n')
    tm2,btr2,bbtr2 = translate_g(m)
    stderr.write(O+'+++'+W+' Google\n')
    stderr.write(tm2 +'\n')
    stderr.write('['+btr2 +']\n')
    stderr.write('['+bbtr2 +']\n')

    try:
      tm3e = bulkpof.find(m)
      tm3 = tm3e.msgstr
      bbtr3 = tm3e.comment
      stderr.write(O+'+++'+W+' bulk\n')
      stderr.write(tm3 + '\n')
      stderr.write('[' + bbtr3 + ']\n')
    except AttributeError:
      tm3 = '*not found*'
      bbtr3 = ''

    rep = getpass.getpass('\n'+B+'==>'+W+' Choose a/b/g/n:')
    if rep == 'n' or not rep in ['a', 'g', 'n', 'b']:
        return m

    if rep == 'a':
        good = tm
        goodsrc = 'from Argos'
        goodbbtr = bbtr
    elif rep == 'b':
        good = tm3
        goodsrc = 'bulk translation'
        goodbbtr = bbtr3
    else:
        good = tm2
        goodsrc = 'from Google'
        goodbbtr = bbtr2

    # translators often drop leading and trailing whitespace
    if m.startswith(' ') and not good.startswith(' '):
      good = ' ' + good
    if m.endswith(' ') and not good.endswith(' '):
      good = good + ' '

    e = polib.POEntry(msgid=m, msgstr=good, comment='[' + goodbbtr + ']', tcomment=goodsrc)
    pof.append(e)
    pof.save()
    return good
