Callback Test Suite
-------------------


CONFIGURE SUITE

  needs lua: edit config.lua and run "make config".


DESCRIPTION

Generates a set of callback invokers in C using lua as a preprocessor.

The invokers pass random arguments to the callback to test, from a set of
reference values which are argument type- and position-specific.


The Callback Invocation Body

Example:
The body for a signature of type  "dpdf)p" at case id 19 is:

void f19(void* addr) 
{ 
  V_p[4] = ((p(*)(d,p,d,f))addr)(K_d[0],K_p[1],K_d[2],K_f[3]);
}                          ^^^^- dyncallback object
             ^^^^^^^^^^^^^- signature
     ^- return type                ^- args from reference values (to be retrieved in handler)


The K_? values are the reference values, which are supposed to be copied to
V_?, and are compared for identity after invocation. The arguments are copied
to V_? in the callback handler, called through "addr".

The reference values stored in K_? are generated once, randomly, at startup.


Specific calling conventions:

Specify 'api' and 'ccprefix' accordingly to generate callbacks for a
specific/custom calling convention:

"__stdcall"  "_s"
"__fastcall" "_f" for gcc compiler
             "_F" for microsoft compiler

See the dyncall documentation for other/more calling convention prefixes.

