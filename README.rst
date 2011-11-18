Sparse Vector Test Program
==========================

This benchmark program compares sparse vector libraries.

Require
-------

- pficommon (https://github.com/pfi/pficommon)
- eigen3 or eigen2 (http://eigen.tuxfamily.org/) [opitonal]
- boost/ublas (http://www.boost.org/doc/libs/1_48_0/libs/numeric/ublas/doc/index.htm) [optional]
- dag_vector (https://github.com/pfi/dag_vector) [optional]

How to run?
-----------

::

 $ ./waf configure
 $ ./waf
 $ ./build/sparse-test
