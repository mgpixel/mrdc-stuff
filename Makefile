all:
	gcc mrdc.c libstem_gamepad_1.4.2/library/release-linux64/libstem_gamepad.a -lpthread gattlib/lib/libgattlib.so -o mrdc

clean:
	rm mrdc