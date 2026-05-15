#!/usr/bin/env python
import sys
from subprocess import Popen, PIPE

# Script to show the new service mode based on stdin/stdout
# requires highlight 4.6

# Add for custom paths: '-D', '/usr/share/highlight/'
p = Popen( ["highlight",'--service-mode', '--out-format', 'xterm256', '-f'],
		  stdin=PIPE, stdout=PIPE, stderr=PIPE, bufsize=1, universal_newlines=True )

endtag = '<endtag/>'

while True:
    filename = input("\nPlease enter the filename (or exit): ")

    if filename=='exit':
        break

    p.stdin.write(str('eof=\x01;tag='+endtag+';syntax='+filename+'\n'))
    p.stdin.flush()

    # write file
    with open(filename) as file:
        while line := file.readline():
            p.stdin.write(line)

    # send end of file delimiter and flush
    p.stdin.write(str('\n\x01\n'))
    p.stdin.flush()

    # read highlight output
    while True:
        result = p.stdout.readline().strip()

        if result==endtag:
            break

        print(result)


for pipe in [p.stdin, p.stdout]:
    try:
        pipe.close()
    except OSError:
        pass

sys.exit(p.wait())
