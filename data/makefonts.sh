#!/bin/sh
for i in *.fnt; do
../fnttobdf <$i | bdftopcf > `basename $i .fnt`.pcf
../fnttobdf -tr <$i | bdftopcf > `basename $i .fnt`-iso.pcf
done
mkfontdir .
xset fp+ `pwd`
xset fp rehash
