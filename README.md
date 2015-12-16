# Compiling

Prequisites: emsdk - I used emsdk_portable

https://kripken.github.io/emscripten-site/docs/getting_started/downloads.html
https://s3.amazonaws.com/mozilla-games/emscripten/releases/emsdk-portable.tar.gz

1. source ~/dev/emsdk_portable/emsdk_set_env.sh
2. make -f makefile.emscripten

# Running

	vagrant@precise64:~/dev/jv7$ node a.out.js
	Calling stub instead of signal()
	J7 Copyright (c) 1990-1993, Iverson Software Inc.  All Rights Reserved.

	1+1
	   2

# Explanation of patches

a.out.patch is needed since function pointers were overloaded and emscripten got confused. Without it, this happens:


	vagrant@precise64:~/dev/jv7$ node a.out.js                                                                                                
	Calling stub instead of signal()                                                                                                          
	J7 Copyright (c) 1990-1993, Iverson Software Inc.  All Rights Reserved.                                                                   
																		  
	1+1                                                                                                                                       
	Invalid function pointer '42' called with signature 'iiii'. Perhaps this is an invalid value (e.g. caused by calling a virtual method on a
	 NULL pointer)? Or calling a function with an incorrect type, which will fail? (it is worth building your source files with -Werror (warni
	ngs are errors), as warnings can indicate undefined behavior which can cause this)                                                        
	This pointer might make sense in another type signature: iii: asm['_plus']  ii: 0  viii: 0  viiii: undefined  vii: 0  vi: 0               
	42                                                                                                                                        
	42                                                                                                                                        
