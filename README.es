sistema de compartición de archivos
===================================


compilar y correr
-----------------

	$ make
	...
	...
	$ # en windows (Cygwin o MSYS2)
	$ ./win32/main.exe <kazaa-dir> <trash-dir> <tmp-dir> <group> <port>
	...
	$ # en linux
	$ ./linux/main <kazaa-dir> <trash-dir> <tmp-dir> <group> <port>
	...
	...
	$ # compilar en la carpeta dist
	$ make dstdir=dist


¿qué hace?
----------

comparte archivos en la carpeta `<kazaa-dir>` con otros clientes,
si se borra algún archivo en `<kazaa-dir>`, este aparece en `<trash-dir>`.

el programa no sobreescribe archivos en `<trash-dir>`, sino que los
guarda con otro nombre (por ejemplo `foo.txt.1` en vez de `foo.txt`).


¿como funciona? y el porqué de 3 directorios
--------------------------------------------

cada vez que se agrega o modifica un archivo en `<kazaa-dir>`, el programa
guarda una copia en `<tmp-dir>`. cuando un archivo se borra en `<kazaa-dir>`,
su copia en `<tmp-dir>` se mueve a `<trash-dir>`
