#include "python_bridge.h"
#include <Python.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>

PyMODINIT_FUNC PyInit_interm(void);

im_result_t im_python_init() {

    if (PyImport_AppendInittab("interm", PyInit_interm) == -1) return IM_ERR;
    Py_Initialize();
    // Safety: only allow absolute path for interm packages if needed,
    // but for plugins we need some path.
    return IM_OK;
}

im_result_t im_python_shutdown() {
    Py_Finalize();
    return IM_OK;
}

im_result_t im_python_execute_file(const char* path) {
    FILE* fp = fopen(path, "r");
    if (!fp) return IM_ERR_IO;
    PyRun_SimpleFile(fp, path);
    fclose(fp);
    return IM_OK;
}

im_result_t im_python_load_plugins(const char* dir) {
    DIR *d; struct dirent *dir_ent; d = opendir(dir);
    if (d) {
        while ((dir_ent = readdir(d)) != NULL) {
            if (strstr(dir_ent->d_name, ".py")) {
                char full_path[512];
                snprintf(full_path, sizeof(full_path), "%s/%s", dir, dir_ent->d_name);
                im_python_execute_file(full_path);
            }
        }
        closedir(d);
    }
    return IM_OK;
}
