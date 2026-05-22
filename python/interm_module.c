#include <Python.h>
#include "buffer.h"
#include "state.h"
#include "commands.h"

static PyObject* method_insert(PyObject* self, PyObject* args) {
    (void)self;
    long pos;
    const char* text;
    if (!PyArg_ParseTuple(args, "ls", &pos, &text)) return NULL;

    im_editor_state_t* state = im_state_get();
    if (state->active_buffer) {
        im_buffer_insert(state->active_buffer, (size_t)pos, text, strlen(text));
    }
    Py_RETURN_NONE;
}

static PyObject* method_get_text(PyObject* self, PyObject* args) {
    (void)self;
    long pos, len;
    if (!PyArg_ParseTuple(args, "ll", &pos, &len)) return NULL;

    im_editor_state_t* state = im_state_get();
    if (!state->active_buffer) Py_RETURN_NONE;

    char* text = im_buffer_get_range(state->active_buffer, (size_t)pos, (size_t)len);
    if (!text) Py_RETURN_NONE;

    PyObject* py_text = PyUnicode_FromString(text);
    free(text);
    return py_text;
}

static PyMethodDef IntermMethods[] = {
    {"insert", method_insert, METH_VARARGS, "Insert text into active buffer"},
    {"get_text", method_get_text, METH_VARARGS, "Get text from active buffer"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef interm_module = {
    PyModuleDef_HEAD_INIT,
    "interm",
    "INTERM Core API",
    -1,
    IntermMethods,
    NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC PyInit_interm(void) {
    return PyModule_Create(&interm_module);
}
