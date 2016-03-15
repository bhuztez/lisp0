=====
LISP0
=====

A bare minimum remake of origin LISP on Linux x86-64.

Run example [#]_

.. code::

    $ make
    $ cat example.in | ./lisp0.elf | diff example.out -

It is static linked without crt, using TLSF [#]_ memory allocator, and
one-pass mark-sweep garbage collector [#]_ . Short symbols, less than
8 bytes, are base64 decoded to fit in 6 bytes.

.. [#] Example code are taken from The Roots of Lisp http://lib.store.yahoo.net/lib/paulgraham/jmc.ps
.. [#] TLSF: a New Dynamic Memory Allocator for Real-Time Systems http://www.gii.upv.es/tlsf/files/ecrts04_tlsf.pdf
.. [#] One Pass Real-Time Generational Mark-Sweep Garbage Collection http://www.erlang.se/publications/memory1995.ps
