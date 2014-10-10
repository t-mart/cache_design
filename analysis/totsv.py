import os
import os.path

files = ['astar.trace', 'bzip2.trace', 'mcf.trace', 'perlbench.trace']

with open("accesses.tsv", 'w') as j:
    j.write("\t".join(["i", "rw", "addr", "filename"]))
    j.write("\n")
    for filename in files:
        last_addr = None
        full_filename = os.path.join("traces",filename)
        with open(full_filename, 'r') as f:
            for i, line in enumerate(f):
                rw, addr = line.split()
                addr = int(addr, 16)
                if addr < 2**30:
                    j.write("\t".join([str(thing) for thing in [i,rw,addr,filename]]))
                    j.write("\n")
                if i > 100000:
                    break
