#ifndef INHERITANCETEST_H
#define INHERITANCETEST_H

#include "ap_int.h"
#include <assert.h>

#include "ProcessBase.hh"
#include "ProcessDerived.hh"
#include "ProcessDerivedTwo.hh"

void InheritanceTest(int*, int*, int*, int*, int*, int*, int*, int*,
					 ap_uint<5>*,
					 int*, int*);

#endif