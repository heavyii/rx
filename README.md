
# Xmodem protocol receiver

*tty2tty* is a tool to creat a pair of lookback virsual tty

*rx* Xmodem protocol receiver

## usage

first run tty2tty to creat a pair of lookback tty

minicom -D /dev/pts/x -b 115200

rx /dev/pts/xx recv.txt
