#!/bin/sh
#
# This generates the tags file for vim or emacs. (vim -t igbinary_serialize8)
#
# Try with "vim -t igbinary_serialize8"
#

find . -name "*.h" -o -name "*.c" | ctags-exuberant --language-force=c -L -
