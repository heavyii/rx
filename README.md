
# Xmodem protocol receiver

*tty2tty* is a tool to creat a pair of virsual tty lookback device

*rx* Xmodem protocol receiver

## usage

first run tty2tty to creat a pair of lookback tty device

minicom -D /dev/pts/x -b 115200

rx /dev/pts/xx recv.txt
