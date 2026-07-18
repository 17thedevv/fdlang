b=open('examples/ex.fl', 'rb').read()
errs=[4336, 4341, 5542, 5872, 6098]
lines=b.split(b'\n')
o=0
for i,l in enumerate(lines):
    for e in errs:
        if o <= e < o+len(l)+1:
            print(f'byte {e} -> line {i+1}: {l.decode(errors="replace").strip()[:80]}')
    o += len(l) + 1
