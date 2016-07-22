all : p2p.x


p2p.x : p2p.chpl 
	chpl -o $@ -O --fast $<


clean :
	-rm -f *.x *~ *_real
