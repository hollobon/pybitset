/*
 * Python bitset module.
 *
 * (C) Pete Hollobon <python@hollobon.com>
 * Based on Objects/setobject.c from Python 2.5.4 / 2.6.2
 *
 */

#include <Python.h>
#include "structmember.h"

PyAPI_DATA(PyTypeObject) bitset_BitsetType;

#ifndef Py_TYPE
/* new in 2.6 */
#define Py_TYPE(ob)		(((PyObject*)(ob))->ob_type)
#endif

#define bitset_BitSet_CheckExact(ob) (Py_TYPE(ob) == &bitset_BitsetType)
#define bitset_Bitset_Check(ob) \
	(Py_TYPE(ob) == &bitset_BitsetType || \
	PyType_IsSubtype(Py_TYPE(ob), &bitset_BitsetType))

typedef struct {
    PyObject_HEAD
    unsigned int bits;
} bitset_BitsetObject;

static void
Bitset_dealloc(bitset_BitsetObject* self)
{
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Bitset_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    bitset_BitsetObject *self;

    self = (bitset_BitsetObject *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->bits = 0U;
    }

    return (PyObject *)self;
}

static int
bitset_read_bits_from_sequence(PyObject *obj, unsigned int *bits)
{
    PyObject *key, *it;
    unsigned int value;
    int error = 0;

	it = PyObject_GetIter(obj);
	if (it == NULL)
		return -1;

    while ((key = PyIter_Next(it)) != NULL) {
        if (!PyInt_Check(key))
            error = 1;

        value = PyInt_AsLong(key);

        if (value < 1 || value > 32)
            error = 1;

        if (error) {
            PyErr_SetString(PyExc_TypeError, "bitsets can only contain integers [1..32]");
            return -1;
        }

        *bits |= 1 << (value - 1);
        Py_DECREF(key);
    }
    Py_DECREF(it);

    if (PyErr_Occurred())
        return -1;

    return 0;
}

static int
Bitset_init(bitset_BitsetObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *arg = NULL;

    if (!PyArg_ParseTuple(args, "|O", &arg))
        return -1;

    if (arg == NULL)
        return 0;

    if (bitset_read_bits_from_sequence(arg, &(self->bits))) {
        self->bits = 0;
        return -1;
    }

    return 0;
}

static PyObject *
bitset_Bitset_copy(bitset_BitsetObject *bso)
{
	bitset_BitsetObject *result = (bitset_BitsetObject *)Bitset_new(&bitset_BitsetType, NULL, NULL);
    result->bits = bso->bits;
    return (PyObject *)result;
}

PyDoc_STRVAR(copy_doc, "Return a copy of a bitset.");

static PyObject *
Bitset_repr(bitset_BitsetObject *bso)
{
	PyObject *keys, *result=NULL, *listrepr;
	int status = Py_ReprEnter((PyObject*)bso);

	if (status != 0) {
		if (status < 0)
			return NULL;
		return PyString_FromFormat("%s(...)", bso->ob_type->tp_name);
	}

	keys = PySequence_List((PyObject *)bso);
	if (keys == NULL)
		goto done;
	listrepr = PyObject_Repr(keys);
	Py_DECREF(keys);
	if (listrepr == NULL)
		goto done;

	result = PyString_FromFormat("%s(%s)", bso->ob_type->tp_name,
		PyString_AS_STRING(listrepr));
	Py_DECREF(listrepr);
done:
	Py_ReprLeave((PyObject*)bso);
	return result;
}

int
bitset_pop(unsigned int *bits)
{
    unsigned v = *bits;
    unsigned int c;     // c will be the number of zero bits on the right,

	if (*bits == 0) {
        return 0;
	}

    /* http://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightBinSearch */
    if (v & 0x1) {
        // special case for odd v (assumed to happen half of the time)
        c = 0;
    }
    else {
        c = 1;
        if ((v & 0xffff) == 0) {
            v >>= 16;
            c += 16;
        }
        if ((v & 0xff) == 0) {
            v >>= 8;
            c += 8;
        }
        if ((v & 0xf) == 0) {
            v >>= 4;
            c += 4;
        }
        if ((v & 0x3) == 0) {
            v >>= 2;
            c += 2;
        }
        c -= v & 0x1;
    }

    (*bits) &= ~0U - (1U << c);
    return c + 1;
}

/***** Bitset iterator type ***********************************************/

typedef struct {
	PyObject_HEAD
	bitset_BitsetObject *bi_bitset; /* Set to NULL when iterator is exhausted */
	unsigned int bi_state;
} bitset_Bitset_iterobject;

static void
bitset_Bitset_iter_dealloc(bitset_Bitset_iterobject *bsi)
{
	Py_XDECREF(bsi->bi_bitset);
	PyObject_GC_Del(bsi);
}

static PyMethodDef bitset_Bitset_iter_methods[] = {
    /*	{"__length_hint__", (PyCFunction)setiter_len, METH_NOARGS, length_hint_doc},*/
 	{NULL,		NULL}		/* sentinel */
};

static PyObject *bitset_Bitset_iter_iternext(bitset_Bitset_iterobject *bi)
{
	bitset_BitsetObject *bso = bi->bi_bitset;
    unsigned int v;

	if (bso == NULL)
		return NULL;
	assert (bitset_Bitset_Check(bso));

    v = bitset_pop(&(bi->bi_state));
    if (v > 0)
        return PyInt_FromLong(v);

	Py_DECREF(bso);
	bi->bi_bitset = NULL;
	return NULL;
}

static PyTypeObject bitset_Bitset_iter_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                                         /* ob_size */
	"Bitset_iterator",                         /* tp_name */
	sizeof(bitset_Bitset_iterobject),          /* tp_basicsize */
	0,                                         /* tp_itemsize */
    /* methods */
	(destructor)bitset_Bitset_iter_dealloc,    /* tp_dealloc */
	0,                                         /* tp_print */
	0,                                         /* tp_getattr */
	0,                                         /* tp_setattr */
	0,                                         /* tp_compare */
	0,                                         /* tp_repr */
	0,                                         /* tp_as_number */
	0,                                         /* tp_as_sequence */
	0,                                         /* tp_as_mapping */
	0,                                         /* tp_hash */
	0,                                         /* tp_call */
	0,                                         /* tp_str */
	PyObject_GenericGetAttr,                   /* tp_getattro */
	0,                                         /* tp_setattro */
	0,                                         /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,                        /* tp_flags */
 	0,                                         /* tp_doc */
 	0,                                         /* tp_traverse */
 	0,                                         /* tp_clear */
	0,                                         /* tp_richcompare */
	0,                                         /* tp_weaklistoffset */
	PyObject_SelfIter,                         /* tp_iter */
	(iternextfunc)bitset_Bitset_iter_iternext, /* tp_iternext */
	bitset_Bitset_iter_methods,                /* tp_methods */
	0,
};

static PyObject *
bitset_Bitset_iter(bitset_BitsetObject *bso)
{
	bitset_Bitset_iterobject *bi = PyObject_GC_New(bitset_Bitset_iterobject, &bitset_Bitset_iter_Type);
	if (bi == NULL)
		return NULL;

	Py_INCREF(bso);
	bi->bi_bitset = bso;
	bi->bi_state = bso->bits;

	return (PyObject *)bi;
}

static Py_ssize_t
bitset_len(PyObject *bso)
{
    unsigned int c;
    unsigned int v = ((bitset_BitsetObject *)bso)->bits;

    /* http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel */
    v = v - ((v >> 1) & 0x55555555);                    // reuse input as temporary
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);     // temp
    c = (((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24; // count
    return c;
}

static int
bitset_contains(bitset_BitsetObject *bso, PyObject *key)
{
    if (!PyInt_Check(key)) {
        PyErr_SetString(PyExc_TypeError, "bitsets can only contain integers [1..32]");
        return -1;
    }

    return (bso->bits & (1 << (PyInt_AsLong(key) - 1))) != 0;
}

static PyMethodDef bitset_methods[] = {
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static PySequenceMethods bitset_as_sequence = {
    bitset_len,                  /* sq_length */
    0,                           /* sq_concat */
    0,                           /* sq_repeat */
    0,                           /* sq_item */
    0,                           /* sq_slice */
    0,                           /* sq_ass_item */
    0,                           /* sq_ass_slice */
    (objobjproc)bitset_contains, /* sq_contains */
};

static PyObject *
bitset_Bitset_add(bitset_BitsetObject *bso, PyObject *key)
{
    if (!PyInt_Check(key) || PyInt_AsLong(key) > 32) {
        PyErr_SetString(PyExc_TypeError, "bitsets can only contain integers [1..32]");
        return NULL;
    }

    bso->bits |= 1 << (PyInt_AsLong(key) - 1);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(add_doc,
"Add an element to a Bitset.\n\
\n\
This has no effect if the element is already present.");

static PyObject *
bitset_Bitset_clear(bitset_BitsetObject *bso)
{
    bso->bits = 0;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(clear_doc, "Remove all elements from this Bitset.");

static PyObject *
bitset_Bitset_update(bitset_BitsetObject *bso, PyObject *other)
{
	if (bitset_Bitset_Check(other)) {
		bso->bits |= ((bitset_BitsetObject *)other)->bits;
        Py_RETURN_NONE;
    }

    if (bitset_read_bits_from_sequence(other, &(bso->bits)))
        return NULL;

	Py_RETURN_NONE;
}

PyDoc_STRVAR(update_doc,
"Update a bitset with the union of itself and another.");

static PyObject *
bitset_Bitset_remove(bitset_BitsetObject *bso, PyObject *key)
{
    if (!PyInt_Check(key) || PyInt_AsLong(key) > 32) {
        PyErr_SetString(PyExc_TypeError, "bitsets can only contain integers [1..32]");
        return NULL;
    }

    if (!(bso->bits & (1 << (PyInt_AsLong(key) - 1)))) {
        PyErr_SetObject(PyExc_KeyError, key);
        return NULL;
    }

    ((bitset_BitsetObject *)bso)->bits &= (~0U - (1 << (PyInt_AsLong(key) - 1)));
	Py_RETURN_NONE;
}

PyDoc_STRVAR(remove_doc,
"Remove an element from a bitset; it must be a member.\n\
\n\
If the element is not a member, raise a KeyError.");


static PyObject *
bitset_Bitset_discard(bitset_BitsetObject *bso, PyObject *key)
{
    if (!PyInt_Check(key) || PyInt_AsLong(key) > 32) {
        PyErr_SetString(PyExc_TypeError, "bitsets can only contain integers [1..32]");
        return NULL;
    }

    bso->bits &= (~0U - (1 << (PyInt_AsLong(key) - 1)));
	Py_RETURN_NONE;
}

PyDoc_STRVAR(discard_doc,
"Remove an element from a bitset if it is a member.\n\
\n\
If the element is not a member, do nothing.");

static PyObject *
bitset_Bitset_pop(bitset_BitsetObject *bso)
{
	if (bso->bits == 0) {
		PyErr_SetString(PyExc_KeyError, "pop from an empty bitset");
		return NULL;
	}

    return PyInt_FromLong(bitset_pop(&(bso->bits)));
}

PyDoc_STRVAR(pop_doc, "Remove and return an arbitrary bitset element.");

static PyObject *
bitset_Bitset_issuperset(bitset_BitsetObject *bso, PyObject *other)
{
    unsigned int otherbits = 0;

    if (!bitset_Bitset_Check(other)) {
        if (bitset_read_bits_from_sequence(other, &otherbits))
            return NULL;
    }
    else {
        otherbits = ((bitset_BitsetObject *)other)->bits;
    }

	if (((bso->bits | otherbits) == bso->bits) && (bso->bits >= otherbits))
        Py_RETURN_TRUE;

    Py_RETURN_FALSE;
}

PyDoc_STRVAR(issuperset_doc, "Report whether this bitset contains another bitset.");

static PyObject *
bitset_Bitset_issubset(bitset_BitsetObject *bso, PyObject *other)
{
    unsigned int otherbits = 0;

    if (!bitset_Bitset_Check(other)) {
        if (bitset_read_bits_from_sequence(other, &otherbits))
            return NULL;
    }
    else {
        otherbits = ((bitset_BitsetObject *)other)->bits;
    }

	if (((bso->bits | otherbits) == otherbits) && (bso->bits <= otherbits))
        Py_RETURN_TRUE;

    Py_RETURN_FALSE;
}

PyDoc_STRVAR(issubset_doc, "Report whether another bitset contains this bitset.");

static PyObject *
bitset_Bitset_isdisjoint(bitset_BitsetObject *bso, PyObject *other)
{
    if ((bso->bits & ((bitset_BitsetObject *)other)->bits) == 0)
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

PyDoc_STRVAR(isdisjoint_doc,
"Return True if two bitsets have a null intersection.");

static PyObject *
bitset_Bitset_difference_update(bitset_BitsetObject *bso, PyObject *other)
{
    unsigned int otherbits = 0;

	if (bitset_Bitset_Check(other)) {
		bso->bits &= ~0UL - ((bitset_BitsetObject *)other)->bits;
        Py_RETURN_NONE;
    }

    if (bitset_read_bits_from_sequence(other, &otherbits))
        return NULL;

    bso->bits &= ~0UL - otherbits;

    Py_RETURN_NONE;
}

PyDoc_STRVAR(difference_update_doc,
"Remove all elements of another bitset from this bitset.");

static PyObject *
bitset_Bitset_difference(bitset_BitsetObject *bso, PyObject *other)
{
	bitset_BitsetObject *result;

	result = (bitset_BitsetObject *)bitset_Bitset_copy(bso);
	if (result == NULL)
		return NULL;

    if (bitset_Bitset_difference_update(result, other) == NULL) {
        Py_DECREF(result);
        return NULL;
    }

	return (PyObject *)result;
}

PyDoc_STRVAR(difference_doc,
"Return the difference of two bitsets as a new bitset.\n\
\n\
(i.e. all elements that are in this bitset but not the other.)");

static PyObject *
bitset_Bitset_symmetric_difference_update(bitset_BitsetObject *bso, PyObject *other)
{
    unsigned int otherbits = 0;

	if (bitset_Bitset_Check(other)) {
		bso->bits ^= ((bitset_BitsetObject *)other)->bits;
        Py_RETURN_NONE;
    }

    if (bitset_read_bits_from_sequence(other, &otherbits))
        return NULL;

    bso->bits ^= otherbits;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(symmetric_difference_update_doc,
"Update a bitset with the symmetric difference of itself and another.");

static PyObject *
bitset_Bitset_symmetric_difference(bitset_BitsetObject *bso, PyObject *other)
{
	bitset_BitsetObject *result;

	result = (bitset_BitsetObject *)bitset_Bitset_copy(bso);
	if (result == NULL)
		return NULL;

    if (bitset_Bitset_symmetric_difference_update(result, other) == NULL) {
        Py_DECREF(result);
        return NULL;
    }

	return (PyObject *)result;
}

PyDoc_STRVAR(symmetric_difference_doc,
"Return the symmetric difference of two bitsets as a new bitset.\n\
\n\
(i.e. all elements that are in exactly one of the bitsets.)");

static PyObject *
bitset_Bitset_union(bitset_BitsetObject *bso, PyObject *other)
{
	bitset_BitsetObject *result;

	result = (bitset_BitsetObject *)bitset_Bitset_copy(bso);
	if (result == NULL)
		return NULL;

    if (bitset_Bitset_update(result, other) == NULL) {
        Py_DECREF(result);
        return NULL;
    }

	return (PyObject *)result;
}

PyDoc_STRVAR(union_doc,
 "Return the union of two bitsets as a new bitset.\n\
\n\
(i.e. all elements that are in either bitset.)");

static PyObject *
bitset_Bitset_intersection_update(bitset_BitsetObject *bso, PyObject *other)
{
    unsigned int otherbits = 0;

	if (bitset_Bitset_Check(other)) {
		bso->bits &= ((bitset_BitsetObject *)other)->bits;
        Py_RETURN_NONE;
    }

    if (bitset_read_bits_from_sequence(other, &otherbits))
        return NULL;

    bso->bits &= otherbits;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(intersection_update_doc,
"Update a bitset with the intersection of itself and another.");

static PyObject *
bitset_Bitset_intersection(bitset_BitsetObject *bso, PyObject *other)
{
	bitset_BitsetObject *result;

	result = (bitset_BitsetObject *)bitset_Bitset_copy(bso);
	if (result == NULL)
		return NULL;

    if (bitset_Bitset_intersection_update(result, other) == NULL) {
        Py_DECREF(result);
        return NULL;
    }

	return (PyObject *)result;
}

PyDoc_STRVAR(intersection_doc,
"Return the intersection of two bitsets as a new bitset.\n\
\n\
(i.e. all elements that are in both bitsets.)");

static PyObject *
bitset_Bitset_reduce(bitset_BitsetObject *bso)
{
    PyObject *result, *args, *state;

    args = PyTuple_New(0);

    state = PyInt_FromLong(bso->bits);
    result = PyTuple_Pack(3, bso->ob_type, args, state);

    Py_XDECREF(args);
    Py_XDECREF(state);
    return result;
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyObject *
bitset_Bitset_setstate(bitset_BitsetObject *bso, PyObject *state)
{
    if (!PyInt_Check(state)) {
		PyErr_SetString(PyExc_TypeError, "Invalid state in __setstate__");
        return NULL;
    }

    bso->bits = PyInt_AsLong(state);

    Py_RETURN_NONE;
}

PyDoc_STRVAR(setstate_doc, "Sets state information.");

static PyMethodDef bitset_Bitset_methods[] = {
	{"add",                         (PyCFunction)bitset_Bitset_add,
     METH_O, add_doc},
	{"clear",                       (PyCFunction)bitset_Bitset_clear,
     METH_NOARGS, clear_doc},
/* 	{"__contains__",                (PyCFunction)bitset_Bitset_direct_contains, */
/*      METH_O | METH_COEXIST, contains_doc}, */
	{"copy",                        (PyCFunction)bitset_Bitset_copy,
     METH_NOARGS, copy_doc},
	{"discard",                     (PyCFunction)bitset_Bitset_discard,
     METH_O, discard_doc},
	{"difference",                  (PyCFunction)bitset_Bitset_difference,
     METH_O, difference_doc},
	{"difference_update",           (PyCFunction)bitset_Bitset_difference_update,
     METH_O, difference_update_doc}, /*  */
	{"intersection",                (PyCFunction)bitset_Bitset_intersection,
     METH_O, intersection_doc},
	{"intersection_update",         (PyCFunction)bitset_Bitset_intersection_update,
     METH_O, intersection_update_doc},
	{"isdisjoint",                  (PyCFunction)bitset_Bitset_isdisjoint,
     METH_O, isdisjoint_doc},
	{"issubset",                    (PyCFunction)bitset_Bitset_issubset,
     METH_O, issubset_doc},
	{"issuperset",                  (PyCFunction)bitset_Bitset_issuperset,
     METH_O, issuperset_doc},
	{"pop",                         (PyCFunction)bitset_Bitset_pop,
     METH_NOARGS, pop_doc},
	{"__reduce__",                  (PyCFunction)bitset_Bitset_reduce,
     METH_NOARGS, reduce_doc},
	{"remove",                      (PyCFunction)bitset_Bitset_remove,
     METH_O, remove_doc},
	{"__setstate__",                (PyCFunction)bitset_Bitset_setstate,
     METH_O, setstate_doc},
	{"symmetric_difference",        (PyCFunction)bitset_Bitset_symmetric_difference,
     METH_O, symmetric_difference_doc},
	{"symmetric_difference_update", (PyCFunction)bitset_Bitset_symmetric_difference_update,
     METH_O, symmetric_difference_update_doc},
/* #ifdef Py_DEBUG */
/* 	{"test_c_api",                  (PyCFunction)test_c_api,                                 */
/*      METH_NOARGS, test_c_api_doc}, */
/* #endif */
	{"union",                       (PyCFunction)bitset_Bitset_union,
     METH_O, union_doc},
	{"update",                      (PyCFunction)bitset_Bitset_update,
     METH_O, update_doc},
	{NULL,		NULL}	            /* sentinel */
};

static PyObject *
bitset_Bitset_sub(bitset_BitsetObject *bso, PyObject *other)
{
	if (!bitset_Bitset_Check(bso) || !bitset_Bitset_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

	return bitset_Bitset_difference(bso, other);
}

static PyObject *
bitset_Bitset_isub(bitset_BitsetObject *bso, PyObject *other)
{
	if (!bitset_Bitset_Check(bso) || !bitset_Bitset_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

    bitset_Bitset_difference_update(bso, other);
    return (PyObject *)bso;
}

static PyObject *
bitset_Bitset_and(bitset_BitsetObject *bso, PyObject *other)
{
	bitset_BitsetObject *result;

	if (!bitset_Bitset_Check(bso) || !bitset_Bitset_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

	result = (bitset_BitsetObject *)bitset_Bitset_copy(bso);
	if (result == NULL)
		return NULL;

    result->bits &= ((bitset_BitsetObject *)other)->bits;

	return (PyObject *)result;
}

static PyObject *
bitset_Bitset_iand(bitset_BitsetObject *bso, PyObject *other)
{
	if (!bitset_Bitset_Check(bso) || !bitset_Bitset_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

    bso->bits &= ((bitset_BitsetObject *)other)->bits;

    return (PyObject *)bso;
}

static PyObject *
bitset_Bitset_xor(bitset_BitsetObject *bso, PyObject *other)
{
	bitset_BitsetObject *result;

	if (!bitset_Bitset_Check(bso) || !bitset_Bitset_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

	result = (bitset_BitsetObject *)bitset_Bitset_copy(bso);
	if (result == NULL)
		return NULL;

    result->bits ^= ((bitset_BitsetObject *)other)->bits;

	return (PyObject *)result;
}

static PyObject *
bitset_Bitset_ixor(bitset_BitsetObject *bso, PyObject *other)
{
	if (!bitset_Bitset_Check(bso) || !bitset_Bitset_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

    bso->bits ^= ((bitset_BitsetObject *)other)->bits;

	return (PyObject *)bso;
}

static PyObject *
bitset_Bitset_or(bitset_BitsetObject *bso, PyObject *other)
{
	bitset_BitsetObject *result;

	if (!bitset_Bitset_Check(bso) || !bitset_Bitset_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

	result = (bitset_BitsetObject *)bitset_Bitset_copy(bso);
	if (result == NULL)
		return NULL;

    result->bits |= ((bitset_BitsetObject *)other)->bits;

	return (PyObject *)result;
}

static PyObject *
bitset_Bitset_ior(bitset_BitsetObject *bso, PyObject *other)
{
	if (!bitset_Bitset_Check(bso) || !bitset_Bitset_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

    bso->bits |= ((bitset_BitsetObject *)other)->bits;

	return (PyObject *)bso;
}

static PyNumberMethods bitset_as_number = {
	0,                              /* nb_add */
	(binaryfunc)bitset_Bitset_sub,	/* nb_subtract */
	0,                              /* nb_multiply */
	0,                              /* nb_divide */
	0,                              /* nb_remainder */
	0,                              /* nb_divmod */
	0,                              /* nb_power */
	0,                              /* nb_negative */
	0,                              /* nb_positive */
	0,                              /* nb_absolute */
	0,                              /* nb_nonzero */
	0,                              /* nb_invert */
	0,                              /* nb_lshift */
	0,                              /* nb_rshift */
    (binaryfunc)bitset_Bitset_and,	/* nb_and */
 	(binaryfunc)bitset_Bitset_xor,	/* nb_xor */
	(binaryfunc)bitset_Bitset_or,	/* nb_or */
	0,                              /* nb_coerce */
	0,                              /* nb_int */
	0,                              /* nb_long */
	0,                              /* nb_float */
	0,                              /* nb_oct */
	0,                              /* nb_hex */
	0,                              /* nb_inplace_add */
	(binaryfunc)bitset_Bitset_isub, /* nb_inplace_subtract */
	0,                              /* nb_inplace_multiply */
	0,                              /* nb_inplace_divide */
	0,                              /* nb_inplace_remainder */
	0,                              /* nb_inplace_power */
	0,                              /* nb_inplace_lshift */
	0,                              /* nb_inplace_rshift */
    (binaryfunc)bitset_Bitset_iand, /* nb_inplace_and */
	(binaryfunc)bitset_Bitset_ixor, /* nb_inplace_xor */
	(binaryfunc)bitset_Bitset_ior,	/* nb_inplace_or */
};

static PyObject *
bitset_Bitset_richcompare(bitset_BitsetObject *v, PyObject *w, int op)
{
	if (!bitset_Bitset_Check(w)) {
		if (op == Py_EQ)
			Py_RETURN_FALSE;
		if (op == Py_NE)
			Py_RETURN_TRUE;
		PyErr_SetString(PyExc_TypeError, "can only compare to a bitset");

		return NULL;
	}
    
	switch (op) {
	case Py_EQ:
        if (v->bits == ((bitset_BitsetObject *)w)->bits)
            Py_RETURN_TRUE;
        Py_RETURN_FALSE;

	case Py_NE:
        if (v->bits != ((bitset_BitsetObject *)w)->bits)
            Py_RETURN_TRUE;
        Py_RETURN_FALSE;

	case Py_LT:
        if (v->bits == ((bitset_BitsetObject *)w)->bits)
            Py_RETURN_FALSE;
	case Py_LE:
		return bitset_Bitset_issubset(v, w);

	case Py_GT:
        if (v->bits == ((bitset_BitsetObject *)w)->bits)
            Py_RETURN_FALSE;
	case Py_GE:
		return bitset_Bitset_issuperset(v, w);
	}

	Py_INCREF(Py_NotImplemented);
	return Py_NotImplemented;
}

static int
bitset_Bitset_nocmp(PyObject *self, PyObject *other)
{
	PyErr_SetString(PyExc_TypeError, "cannot compare bitsets using cmp()");
	return -1;
}

PyDoc_STRVAR(bitset_Bitset_doc,
"Bitset(iterable) --> Bitset object\n\
\n\
Build an unordered set of integers in the range [1,32].");

PyTypeObject bitset_BitsetType = {
    PyObject_HEAD_INIT(NULL)
    0,                                      /* ob_size */
    "bitset.Bitset",                        /* tp_name */
    sizeof(bitset_BitsetObject),            /* tp_basicsize */
    0,                                      /* tp_itemsize */
    (destructor)Bitset_dealloc,             /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    bitset_Bitset_nocmp,                    /* tp_compare */
    (reprfunc)Bitset_repr,                  /* tp_repr */
    &bitset_as_number,                      /* tp_as_number */
	&bitset_as_sequence,                    /* tp_as_sequence */
    0,                                      /* tp_as_mapping */
	(hashfunc)PyObject_HashNotImplemented,	/* tp_hash */
    0,                                      /* tp_call */
    0,                                      /* tp_str */
    0,                                      /* tp_getattro */
    0,                                      /* tp_setattro */
    0,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                     /* tp_flags */
    bitset_Bitset_doc,                      /* tp_doc */
	0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    (richcmpfunc)bitset_Bitset_richcompare, /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
	(getiterfunc)bitset_Bitset_iter,        /* tp_iter */
    0,                                      /* tp_iternext */
    bitset_Bitset_methods,                  /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    0,                                      /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    (initproc)Bitset_init,                  /* tp_init */
	0,                                      /* tp_alloc */
    Bitset_new,                             /* tp_new */
};

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initbitset(void)
{
    PyObject* m;

    bitset_BitsetType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&bitset_BitsetType) < 0)
        return;

    m = Py_InitModule3("bitset", bitset_methods,
                       "Bitset module.");

    if (m == NULL)
        return;

    Py_INCREF(&bitset_BitsetType);
    PyModule_AddObject(m, "Bitset", (PyObject *)&bitset_BitsetType);
}
