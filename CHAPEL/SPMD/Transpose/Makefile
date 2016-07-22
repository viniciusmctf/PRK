all : transpose.x

transpose_sl.x : transpose_sl.chpl
	chpl -o $@ -O --fast $<


transpose.x : transpose.chpl
	chpl -o $@ -O --fast $<


clean: 
	-rm -f *.x *~ *_real
