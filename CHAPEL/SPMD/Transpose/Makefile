OPTS=-snoRefCount

all : transpose.x

transpose.x : transpose.chpl
	chpl -o $@ -O --fast $< ${OPTS}


clean: 
	-rm -f *.x *~ *_real
