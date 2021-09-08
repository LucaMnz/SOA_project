# SOA_project

This project aims to propose a subsystem for managing
the exchange of messages between threads through 
tag-based approach. For this purpose 4 syscalls have 
been developed. A thread can therefore create and 
open a certain service to which a specific tag is 
associated and on this it will be possible to 
receive and / or send messages. There are also 
control operations such as the awakening of threads 
and also the removal of a service. 

To carry out tests on the functioning of the system 
a special 'demo' module is provided which carries 
out operations so that these highlight the correct behavior of the 
system itself. It is possible to setup the system 
using the command ./install.sh which will take care 
of the initialization and will execute the make file.
The install.sh file will also compile 'demo.c' and 
to run it it's needed to run ./demo after have moved in
its directory. To uninstall the syscall previusly added, 
deallocate and clean the directory just run ./uninstall.sh .

For the sake of completeness, it should be noted 
that the code is developed starting from examples 
seen during the Advanced Operating Systems course and it has 
been tested on a Virtual Machine with Ubuntu 20.04.3 LTS and
Kernel 5.11.0-27-generic
