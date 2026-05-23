#include <Python.h>
#include "buffer.h"
#include "state.h"
#include "commands.h"
#include "event.h"

static PyObject* g_python_callbacks[IM_EVENT_MAX];

static void event_proxy_callback(im_event_t* event, void* user_data) {
    (void)user_data;
    PyGILState_STATE gstate = PyGILState_Ensure();
    PyObject* callback = g_python_callbacks[event->type];
    if (callback && PyCallable_Check(callback)) {
        PyObject* arglist = Py_BuildValue("(i)", (int)event->type);
        PyObject_CallObject(callback, arglist);
        Py_DECREF(arglist);
    }
    PyGILState_Release(gstate);
}

static PyObject* method_insert(PyObject* self, PyObject* args) {
    (void)self;
    long pos;
    const char* text;
    if (!PyArg_ParseTuple(args, "ls", &pos, &text)) return NULL;
    im_editor_state_t* state = im_state_get();
    if (state->active_buffer) im_buffer_insert(state->active_buffer, (size_t)pos, text, strlen(text));
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

static PyObject* method_set_mode(PyObject* self, PyObject* args) {
    (void)self;
    int mode;
    if (!PyArg_ParseTuple(args, "i", &mode)) return NULL;
    im_state_set_mode((im_mode_t)mode);
    Py_RETURN_NONE;
}

static PyObject* method_subscribe(PyObject* self, PyObject* args) {
    (void)self;
    int type;
    PyObject* callback;
    if (!PyArg_ParseTuple(args, "iO", &type, &callback)) return NULL;
    if (!PyCallable_Check(callback)) return NULL;

    if (type >= 0 && type < IM_EVENT_MAX) {
        Py_XINCREF(callback);
        g_python_callbacks[type] = callback;
        im_event_subscribe((im_event_type_t)type, event_proxy_callback, NULL);
    }
    Py_RETURN_NONE;
}

static PyMethodDef IntermMethods[] = {
    {"insert", method_insert, METH_VARARGS, "Insert text"},
    {"get_text", method_get_text, METH_VARARGS, "Get text"},
    {"set_mode", method_set_mode, METH_VARARGS, "Set editor mode"},
    {"subscribe", method_subscribe, METH_VARARGS, "Subscribe to events"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef interm_module = {
    PyModuleDef_HEAD_INIT, "interm", "INTERM Core API", -1, IntermMethods, NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC PyInit_interm(void) {
    PyObject* m = PyModule_Create(&interm_module);
    PyModule_AddIntConstant(m, "MODE_NORMAL", IM_MODE_NORMAL);
    PyModule_AddIntConstant(m, "MODE_INSERT", IM_MODE_INSERT);
    PyModule_AddIntConstant(m, "MODE_VISUAL", IM_MODE_VISUAL);
    PyModule_AddIntConstant(m, "MODE_COMMAND", IM_MODE_COMMAND);
    PyModule_AddIntConstant(m, "EVENT_KEY_PRESS", IM_EVENT_KEY_PRESS);
    return m;
}
