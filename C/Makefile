main: main.c mxml/libmxml4.a
	$(CC) $^ -o $@ -O2 -g0

mxml:
	git clone https://github.com/michaelrsweet/mxml
mxml/libmxml4.a: mxml
	cd mxml; ./configure; make; cd ..
