#!/bin/sh
for i in code*.m4
do
  t=`echo $i|sed s/\.m4/.c/`;
  echo "Translating: $i -> $t"
  m4 < $i >$t
done
