#!/bin/sh

PATH=${PATH}:..

if [ ! -f tos.img ]; then
	echo "Please copy a TOS image to this directory as tos.img"
	exit 1
fi

echo "Making the System fonts..."

tosfonts < tos.img

for i in *.fnt; do
fnttobdf <$i | bdftopcf > `basename $i .fnt`.pcf
fnttobdf -tr <$i | bdftopcf > `basename $i .fnt`-iso.pcf
done

mkfontdir .
xset fp+ `pwd`
xset fp rehash
