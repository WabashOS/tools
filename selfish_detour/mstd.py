#!/usr/bin/env python
import statistics as stat
import sys

print "Mean: " + str(stat.mean(map(int, sys.argv[1:])))
print "STD: " + str(stat.stdev(map(int, sys.argv[1:])))

exit()
