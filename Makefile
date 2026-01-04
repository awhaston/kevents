MODULE_FILENAME=kevents

obj-m +=  $(MODULE_FILENAME).o
KO_FILE=$(MODULE_FILENAME).ko
PROJECT_DIR=git/kevents
BUILD_DIR=$(HOME)/$(PROJECT_DIR)/build

export KROOT=/lib/modules/$(shell uname -r)/build

compile_lib: kevents_lib.c
	gcc -fPIC -c kevents_lib.c
	gcc -shared -o libkevents.so kevents_lib.o


compile_static: kevents_lib.c
	gcc -c kevents_lib.c 
	ar rcs libkevents.a kevents_lib.o

test_apps: compile_static test_app_1.c test_app_2.c
	gcc -g -o test_app_1 test_app_1.c -L. -lkevents
	gcc -g -o test_app_2 test_app_2.c -L. -lkevents

compile: clean
	@$(MAKE) -C $(KROOT) M=$(PWD) modules

modules_install:
	@$(MAKE) -C $(KROOT) M=$(PWD) modules_install

clean: 
	@$(MAKE) -C $(KROOT) M=$(PWD) clean
	rm -rf   Module.symvers modules.order

insert: compile
	sudo insmod $(KO_FILE)

remove:
	sudo rmmod $(MODULE_FILENAME)

printlog:
	sudo dmesg -c 
	sudo insmod $(KO_FILE)
	dmesg

output:
	sudo rmmod $(MODULE_FILENAME)
	sudo dmesg -c
	sudo insmod $(KO_FILE) param1=10 param2=20
	sudo dmesg -c

testsanity: clean compile insert remove
