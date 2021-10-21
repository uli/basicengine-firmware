from __future__ import print_function
import sys
count=0
for i in sys.stdin.readlines():
  print('static const char __ep%d [] PROGMEM = "%s";' % (count, i.strip()))
  count += 1
print('static PROGMEM const char * const epigrams[] = {')
for i in range(0, count):
  print('__ep%d, ' % i,)
print('\n};')
