
all: WINGs.c
	python setup.py build
	ln -s `find build/ -name wings.so` .

install: WINGs.c
	python setup.py install

clean:
	rm -rf build dist WINGs.c *.o *~ *.so MANIFEST wings.py *.pyc *.pyo core

dist:
	python setup.py sdist

WINGs.c: WINGs.i
	swig -python -noproxy `get-wings-flags --cflags` -I.. -o WINGs.c WINGs.i

