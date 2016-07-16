all : transpose.x

transpose.x : transpose.chpl
	chpl -o $@ -O --fast $<


clean: 
	-rm -f *.x *~
