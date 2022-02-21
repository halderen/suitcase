/*
 * Copyright (c) 2021 A.W. van Halderen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define PY_SSIZE_T_CLEAN
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <Python.h>
#include "utilities.h"
#include "settings.h"
#include "modules.h"
#include "logging.h"
#include "executelua.h"

static PyObject*
suitcaseCommand(PyObject *self, PyObject *args)
{
    if(!PyArg_ParseTuple(args, ":numargs"))
        return NULL;
    return PyLong_FromLong(42);
}

static PyMethodDef suitcaseMethods[] = {
    { "suitcase", suitcaseCommand, METH_VARARGS, "Return the number of arguments received by the process." },
    { NULL, NULL, 0, NULL }
};

static PyModuleDef suitcaseModule = {
    PyModuleDef_HEAD_INIT, "suitcase", NULL, -1, suitcaseMethods, NULL, NULL, NULL, NULL
};

static PyObject*
suitcaseEmbed(void)
{
    return PyModule_Create(&suitcaseModule);
}

int
executepython(char* command, va_list ap)
{
    int rcode;
    (void)ap;

    wchar_t *program;

    program = Py_DecodeLocale(argv[0], NULL);
    program = (program ? program : argv[0]);
    Py_SetProgramName(program);
    PyImport_AppendInittab("suitcase", &suitcaseEmbed);
    Py_Initialize();
    PyRun_SimpleString(command);
    if (Py_FinalizeEx() < 0) {
        goto failure;
    }
    PyMem_RawFree(program);
    return 0;

  failure:
    PyMem_RawFree(program);
    PyErr_Print();
    return 1;
}
