a$="123äöü7890"
for i=0 to 20:?left$(a$,i);"=":next
for i=0 to 20:?right$(a$,i);"=":next
for i=0 to 15:?"m";i:for j=0 to 15:?mid$(a$,i, j);"=":next:next

a$="1234567890"
for i=0 to 20:?bleft$(a$,i);"=":next
for i=0 to 20:?bright$(a$,i);"=":next
for i=0 to 15:?"m";i:for j=0 to 15:?bmid$(a$,i, j);"=":next:next
