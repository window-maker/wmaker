%module wings
%{
#include "WINGs/WINGsP.h"
%}

%include typemaps.i

// This tells SWIG to treat char ** as a special case
%typemap(python, in) char ** {
    /* Check if is a list */
    if (PyList_Check($input)) {
        int size = PyList_Size($input);
        int i = 0;
        $1 = (char **) wmalloc((size+1)*sizeof(char *));
        for (i = 0; i < size; i++) {
            PyObject *o = PyList_GetItem($input, i);
            if (PyString_Check(o))
                $1[i] = PyString_AsString(PyList_GetItem($input, i));
            else {
                PyErr_SetString(PyExc_TypeError, "list must contain strings");
                wfree($1);
                return NULL;
            }
        }
        $1[i] = 0;
    } else {
        PyErr_SetString(PyExc_TypeError, "not a list");
        return NULL;
    }
}
// This cleans up the char ** array we malloc-ed before the function call
%typemap(python, freearg) char ** {
    wfree($1);
}
// This allows a C function to return a char ** as a Python list
%typemap(python, out) char ** {
    int len,i;
    len = 0;
    while ($1[len]) len++;
    $result = PyList_New(len);
    for (i = 0; i < len; i++) {
        PyList_SetItem($result, i, PyString_FromString($1[i]));
    }
}

// Now for some callbacks
%typemap(python, in) PyObject *pyacArgs {
    if (PyTuple_Check($input)) {
        if (PyTuple_Size($input) != 3) {
            PyErr_SetString(PyExc_ValueError,
                            "wrong number of parameters in tuple. should be 3.");
            return NULL;
        } else {
            PyObject *func = PyTuple_GetItem($input, 1);
            if (func!=Py_None && !PyCallable_Check(func)) {
                PyErr_SetString(PyExc_TypeError,
                                "'action' needs to be a callable object!");
                return NULL;
            }
        }
    } else {
        PyErr_SetString(PyExc_TypeError, "2nd argument not a tuple!");
        return NULL;
    }
    $1 = $input;
}

%typemap(python, in) PyObject *pycArgs {
    if (PyTuple_Check($input)) {
        if (PyTuple_Size($input) != 2) {
            PyErr_SetString(PyExc_ValueError,
                            "wrong number of parameters in tuple. should be 2.");
            return NULL;
        } else {
            PyObject *func = PyTuple_GetItem($input, 0);
            if (func!=Py_None && !PyCallable_Check(func)) {
                PyErr_SetString(PyExc_TypeError,
                                "'action' needs to be a callable object!");
                return NULL;
            }
        }
    } else {
        PyErr_SetString(PyExc_TypeError, "2nd argument not a tuple!");
        return NULL;
    }
    $1 = $input;
}

// Type mapping for grabbing a FILE * from Python
%typemap(python, in) FILE * {
    if (!PyFile_Check($input)) {
        PyErr_SetString(PyExc_TypeError, "Need a file!");
        return NULL;
    }
    $1 = PyFile_AsFile($input);
}

/* These are for free-ing the return of functions that need to be freed
 * before returning control to python. */
%typemap(python, ret) char* WMGetTextFieldText { wfree($1); };


%include exception.i

%exception pyWMScreenMainLoop {
    $function
    if (PyErr_Occurred())
        return NULL;
}

%exception pyWMRunModalLoop {
    $function
    if (PyErr_Occurred())
        return NULL;
}

%{
    static int mainLoopDone = 0;
%}


%inline %{
    WMScreen *pyWMOpenScreen(const char *display, int simpleapp)
    {
        Display *dpy = XOpenDisplay(display);

        if (!dpy) {
            wwarning("WINGs: could not open display %s", XDisplayName(display));
            return NULL;
        }

        if (simpleapp) {
            return WMCreateSimpleApplicationScreen(dpy);
        } else {
            return WMCreateScreen(dpy, DefaultScreen(dpy));
        }
    }

    void pyWMScreenMainLoop(WMScreen *scr)
    {
        XEvent event;

        while (!PyErr_Occurred() && !mainLoopDone) {
            WMNextEvent(((W_Screen*)scr)->display, &event);
            WMHandleEvent(&event);
        }
    }

    void pyWMBreakScreenMainLoop(WMScreen *scr)
    {
        mainLoopDone = 1;
    }

    void pyWMRunModalLoop(WMScreen *scr, WMView *view)
    {
        int oldModalLoop = scr->modalLoop;
        WMView *oldModalView = scr->modalView;

        scr->modalView = view;

        scr->modalLoop = 1;
        while (!PyErr_Occurred() && scr->modalLoop) {
            XEvent event;

            WMNextEvent(scr->display, &event);
            WMHandleEvent(&event);
        }

        scr->modalView = oldModalView;
        scr->modalLoop = oldModalLoop;
    }
%}


//%rename WMScreenMainLoop _WMScreenMainLoop;
//%rename(_WMScreenMainLoop) WMScreenMainLoop;
//%rename WMRunModalLoop _WMRunModalLoop;


%{
    /* These functions match the prototypes of the normal C callback
     * functions. However, we use the clientdata pointer for holding a
     * reference to a Python tuple containing (object, funct, clientData).
     */
    static void PythonWMActionCallback(WMWidget *widget, void *cdata)
    {
        PyObject *pyobj, *func, *pydata, *arglist, *tuple, *result;

        tuple = (PyObject*) cdata;
        pyobj = PyTuple_GetItem(tuple, 0);
        func = PyTuple_GetItem(tuple, 1);
        if (func && func!=Py_None) {
            pydata = PyTuple_GetItem(tuple, 2);
            arglist = Py_BuildValue("(OO)", pyobj, pydata);
            result = PyEval_CallObject(func, arglist);
            Py_DECREF(arglist);
            Py_XDECREF(result);
        }
    }

    static void PythonWMCallback(void *data)
    {
        PyObject *func, *pydata, *arglist, *tuple, *result;

        tuple = (PyObject*) data;
        func = PyTuple_GetItem(tuple, 0);
        if (func && func!=Py_None) {
            pydata = PyTuple_GetItem(tuple, 1);
            arglist = Py_BuildValue("(O)", pydata);
            result = PyEval_CallObject(func, arglist);
            Py_DECREF(arglist);
            Py_XDECREF(result);
        }
    }

    static void
    pyTextFieldDidBeginEditing(WMTextFieldDelegate *self, WMNotification *notif)
    {
        PyObject *pyobj, *delegate, *func, *pydata, *arglist, *tuple, *result;
        int action;

        tuple = (PyObject*) self->data;
        pyobj = PyTuple_GetItem(tuple, 0);
        delegate = PyTuple_GetItem(tuple, 1);
        if (delegate != Py_None) {
            // should we call PyObject_HasAttrString()?? rather not and let
            // python raise an exception because the object doesn't has the
            // attribute
            func = PyObject_GetAttrString(delegate, "didBeginEditing");
            if (func!=NULL && func!=Py_None) {
                pydata = PyObject_GetAttrString(delegate, "data");
                if (!pydata) {
                    Py_INCREF(Py_None);
                    pydata = Py_None;
                }
                action = (int)WMGetNotificationClientData(notif);
                arglist = Py_BuildValue("(OOi)", pyobj, pydata, action);
                result = PyEval_CallObject(func, arglist);
                Py_DECREF(pydata);
                Py_DECREF(arglist);
                Py_XDECREF(result);
            }
            Py_XDECREF(func);
        }
    }

    static void
    pyTextFieldDidChange(WMTextFieldDelegate *self, WMNotification *notif)
    {
        PyObject *pyobj, *delegate, *func, *pydata, *arglist, *tuple, *result;
        int action;

        tuple = (PyObject*) self->data;
        pyobj = PyTuple_GetItem(tuple, 0);
        delegate = PyTuple_GetItem(tuple, 1);
        if (delegate != Py_None) {
            func = PyObject_GetAttrString(delegate, "didChange");
            if (func!=NULL && func!=Py_None) {
                pydata = PyObject_GetAttrString(delegate, "data");
                if (!pydata) {
                    Py_INCREF(Py_None);
                    pydata = Py_None;
                }
                action = (int)WMGetNotificationClientData(notif);
                arglist = Py_BuildValue("(OOi)", pyobj, pydata, action);
                result = PyEval_CallObject(func, arglist);
                Py_DECREF(pydata);
                Py_DECREF(arglist);
                Py_XDECREF(result);
            }
            Py_XDECREF(func);
        }
    }

    static void
    pyTextFieldDidEndEditing(WMTextFieldDelegate *self, WMNotification *notif)
    {
        PyObject *pyobj, *delegate, *func, *pydata, *arglist, *tuple, *result;
        int action;

        tuple = (PyObject*) self->data;
        pyobj = PyTuple_GetItem(tuple, 0);
        delegate = PyTuple_GetItem(tuple, 1);
        if (delegate != Py_None) {
            func = PyObject_GetAttrString(delegate, "didEndEditing");
            if (func!=NULL && func!=Py_None) {
                pydata = PyObject_GetAttrString(delegate, "data");
                if (!pydata) {
                    Py_INCREF(Py_None);
                    pydata = Py_None;
                }
                action = (int)WMGetNotificationClientData(notif);
                arglist = Py_BuildValue("(OOi)", pyobj, pydata, action);
                result = PyEval_CallObject(func, arglist);
                Py_DECREF(pydata);
                Py_DECREF(arglist);
                Py_XDECREF(result);
            }
            Py_XDECREF(func);
        }
    }

    static Bool
    pyTextFieldShouldBeginEditing(WMTextFieldDelegate *self, WMTextField *tPtr)
    {
        PyObject *pyobj, *delegate, *func, *pydata, *arglist, *tuple, *result;
        Bool retval = False;

        tuple = (PyObject*) self->data;
        pyobj = PyTuple_GetItem(tuple, 0);
        delegate = PyTuple_GetItem(tuple, 1);
        if (delegate != Py_None) {
            func = PyObject_GetAttrString(delegate, "shouldBeginEditing");
            if (func!=NULL && func!=Py_None) {
                pydata = PyObject_GetAttrString(delegate, "data");
                if (!pydata) {
                    Py_INCREF(Py_None);
                    pydata = Py_None;
                }
                arglist = Py_BuildValue("(OO)", pyobj, pydata);
                result = PyEval_CallObject(func, arglist);
                if (result!=Py_None && PyInt_AsLong(result)!=0) {
                    retval = True;
                }
                Py_DECREF(pydata);
                Py_DECREF(arglist);
                Py_XDECREF(result);
            }
            Py_XDECREF(func);
        }

        return retval;
    }

    static Bool
    pyTextFieldShouldEndEditing(WMTextFieldDelegate *self, WMTextField *tPtr)
    {
        PyObject *pyobj, *delegate, *func, *pydata, *arglist, *tuple, *result;
        Bool retval = False;

        tuple = (PyObject*) self->data;
        pyobj = PyTuple_GetItem(tuple, 0);
        delegate = PyTuple_GetItem(tuple, 1);
        if (delegate != Py_None) {
            func = PyObject_GetAttrString(delegate, "shouldEndEditing");
            if (func!=NULL && func!=Py_None) {
                pydata = PyObject_GetAttrString(delegate, "data");
                if (!pydata) {
                    Py_INCREF(Py_None);
                    pydata = Py_None;
                }
                arglist = Py_BuildValue("(OO)", pyobj, pydata);
                result = PyEval_CallObject(func, arglist);
                if (result!=Py_None && PyInt_AsLong(result)!=0) {
                    retval = True;
                }
                Py_DECREF(pydata);
                Py_DECREF(arglist);
                Py_XDECREF(result);
            }
            Py_XDECREF(func);
        }

        return retval;
    }
%}

%inline %{
    void pyWMSetWindowCloseAction(WMWindow *win, PyObject *pyacArgs) {
        WMSetWindowCloseAction(win, PythonWMActionCallback, (void*)pyacArgs);
        Py_INCREF(pyacArgs);
    }

    void pyWMSetButtonAction(WMButton *bPtr, PyObject *pyacArgs) {
        WMSetButtonAction(bPtr, PythonWMActionCallback, (void*)pyacArgs);
        Py_INCREF(pyacArgs);
    }

    void pyWMSetScrollerAction(WMScroller *sPtr, PyObject *pyacArgs) {
        WMSetScrollerAction(sPtr, PythonWMActionCallback, (void*)pyacArgs);
        Py_INCREF(pyacArgs);
    }

    void pyWMSetListAction(WMList *lPtr, PyObject *pyacArgs) {
        WMSetListAction(lPtr, PythonWMActionCallback, (void*)pyacArgs);
        Py_INCREF(pyacArgs);
    }

    void pyWMSetListDoubleAction(WMList *lPtr, PyObject *pyacArgs) {
        WMSetListDoubleAction(lPtr, PythonWMActionCallback, (void*)pyacArgs);
        Py_INCREF(pyacArgs);
    }

    void pyWMSetBrowserAction(WMBrowser *bPtr, PyObject *pyacArgs) {
        WMSetBrowserAction(bPtr, PythonWMActionCallback, (void*)pyacArgs);
        Py_INCREF(pyacArgs);
    }

    void pyWMSetBrowserDoubleAction(WMBrowser *bPtr, PyObject *pyacArgs) {
        WMSetBrowserDoubleAction(bPtr, PythonWMActionCallback, (void*)pyacArgs);
        Py_INCREF(pyacArgs);
    }

    void pyWMSetMenuItemAction(WMMenuItem *miPtr, PyObject *pyacArgs) {
        WMSetMenuItemAction(miPtr, PythonWMActionCallback, (void*)pyacArgs);
        Py_INCREF(pyacArgs);
    }

    void pyWMSetPopUpButtonAction(WMPopUpButton *pPtr, PyObject *pyacArgs) {
        WMSetPopUpButtonAction(pPtr, PythonWMActionCallback, (void*)pyacArgs);
        Py_INCREF(pyacArgs);
    }

    void pyWMSetSliderAction(WMSlider *sPtr, PyObject *pyacArgs) {
        WMSetSliderAction(sPtr, PythonWMActionCallback, (void*)pyacArgs);
        Py_INCREF(pyacArgs);
    }

    void pyWMSetRulerMoveAction(WMRuler *rPtr, PyObject *pyacArgs) {
        WMSetRulerMoveAction(rPtr, PythonWMActionCallback, (void*)pyacArgs);
        Py_INCREF(pyacArgs);
    }

    void pyWMSetRulerReleaseAction(WMRuler *rPtr, PyObject *pyacArgs) {
        WMSetRulerReleaseAction(rPtr, PythonWMActionCallback, (void*)pyacArgs);
        Py_INCREF(pyacArgs);
    }

    void pyWMSetColorPanelAction(WMColorPanel *panel, PyObject *pyacArgs) {
        WMSetColorPanelAction(panel, (WMAction2*)PythonWMActionCallback, (void*)pyacArgs);
        Py_INCREF(pyacArgs);
    }

    void* pyWMAddTimerHandler(int miliseconds, PyObject *pycArgs) {
        Py_INCREF(pycArgs);
        return (void*)WMAddTimerHandler(miliseconds, PythonWMCallback,
                                        (void*)pycArgs);
    }

    void* pyWMAddPersistentTimerHandler(int miliseconds, PyObject *pycArgs) {
        Py_INCREF(pycArgs);
        return (void*)WMAddPersistentTimerHandler(miliseconds, PythonWMCallback,
                                                  (void*)pycArgs);
    }

    /* this doesn't work. we pass (func, data) as cdata at creation time, but
     * only data at destruction, so it won't find it unless we change
     * WMDeleteTimerWithClientData() to extract data from the tuple, and this
     * requires access to the internals of WINGs
    void pyWMDeleteTimerWithClientData(PyObject *pycData) {
        WMDeleteTimerWithClientData((void*)pycData);
    }*/

    void pyWMDeleteTimerHandler(void *handlerID) {
        WMDeleteTimerHandler((WMHandlerID)handlerID);
    }

    void* pyWMAddIdleHandler(PyObject *pycArgs) {
        Py_INCREF(pycArgs);
        return (void*)WMAddIdleHandler(PythonWMCallback, (void*)pycArgs);
    }

    void pyWMDeleteIdleHandler(void *handlerID) {
        WMDeleteIdleHandler((WMHandlerID)handlerID);
    }

%}


%exception pyWMSetTextFieldDelegate {
    $function
    if (PyErr_Occurred()) {
        return NULL;
    }
}

%inline %{
    void pyWMSetTextFieldDelegate(WMTextField *tPtr, PyObject *txtArgs) {
        WMTextFieldDelegate *td;

        if (!txtArgs || !PyTuple_Check(txtArgs) || PyTuple_Size(txtArgs)!=2) {
            PyErr_SetString(PyExc_TypeError, "invalid setting of WMTextField "
                            "delegate. Should be '(self, delegate)'");
            return;
        }
        // how do I check if txtArgs[1] is an instance of WMTextFieldDelegate?
        td = WMGetTextFieldDelegate(tPtr);
        if (!td) {
            td = (WMTextFieldDelegate*)wmalloc(sizeof(WMTextFieldDelegate));
            td->didBeginEditing    = pyTextFieldDidBeginEditing;
            td->didChange          = pyTextFieldDidChange;
            td->didEndEditing      = pyTextFieldDidEndEditing;
            td->shouldBeginEditing = pyTextFieldShouldBeginEditing;
            td->shouldEndEditing   = pyTextFieldShouldEndEditing;
        } else {
            Py_XDECREF((PyObject*)td->data);
        }
        Py_INCREF(txtArgs);
        td->data = txtArgs;
        WMSetTextFieldDelegate(tPtr, td);
    }
%}


%inline %{
    PyObject* pyWMGetTextFieldDelegate(WMTextField *tPtr) {
        WMTextFieldDelegate *td;
        PyObject *result, *tuple;

        td = WMGetTextFieldDelegate(tPtr);
        if (!td) {
            Py_INCREF(Py_None);
            return Py_None;
        }

        tuple = (PyObject*)td->data;
        if (!tuple || !PyTuple_Check(tuple) || PyTuple_Size(tuple)!=2) {
            PyErr_SetString(PyExc_TypeError, "invalid TextField delegate");
            return NULL;
        }

        result = PyTuple_GetItem(tuple, 1);
        if (!result)
            result = Py_None;

        Py_INCREF(result);

        return result;
    }
%}


/* ignore structures we will not use */
%ignore ConnectionDelegate;

/* ignore functions we don't need */
// should we ignore vararg functions, or just convert them to functions with
// a fixed number of parameters?
%varargs(char*) wmessage;
//%ignore wmessage;
%ignore wwarning;
%ignore wfatal;
%ignore wsyserror;
%ignore wsyserrorwithcode;
%ignore WMCreatePLArray;
%ignore WMCreatePLDictionary;

%apply int *INPUT { int *argc };

#define Bool int

%include "WINGs/WUtil.h"

/* ignore structures we will not use */

/* ignore functions we don't need */

%include "WINGs/WINGs.h"


%{
void
WHandleEvents()
{
    /* Check any expired timers */
    W_CheckTimerHandlers();

    /* Do idle and timer stuff while there are no input events */
    /* Do not wait for input here. just peek to se if input is available */
    while (!W_HandleInputEvents(False, -1) && W_CheckIdleHandlers()) {
        /* dispatch timer events */
        W_CheckTimerHandlers();
    }

    W_HandleInputEvents(True, -1);

    /* Check any expired timers */
    W_CheckTimerHandlers();
}
%}


/* rewrite functions originally defined as macros */
%inline %{
#undef WMDuplicateArray
WMArray* WMDuplicateArray(WMArray* array) {
    return WMCreateArrayWithArray(array);
}

#undef WMPushInArray
void WMPushInArray(WMArray *array, void *item) {
    WMAddToArray(array, item);
}

#undef WMSetInArray
void* WMSetInArray(WMArray *array, int index, void *item) {
    return WMReplaceInArray(array, index, item);
}

#undef WMRemoveFromArray
int WMRemoveFromArray(WMArray *array, void *item) {
    return WMRemoveFromArrayMatching(array, NULL, item);
}

#undef WMGetFirstInArray
int WMGetFirstInArray(WMArray *array, void *item) {
    return WMFindInArray(array, NULL, item);
}

#undef WMCreateBag
WMBag* WMCreateBag(int size) {
    return WMCreateTreeBag();
}

#undef WMCreateBagWithDestructor
WMBag* WMCreateBagWithDestructor(int size, WMFreeDataProc *destructor) {
    return WMCreateTreeBagWithDestructor(destructor);
}

#undef WMSetInBag
void* WMSetInBag(WMBag *bag, int index, void *item) {
    return WMReplaceInBag(bag, index, item);
}

#undef WMAddItemToTree
WMTreeNode* WMAddItemToTree(WMTreeNode *parent, void *item) {
    return WMInsertItemInTree(parent, -1, item);
}

#undef WMAddNodeToTree
WMTreeNode* WMAddNodeToTree(WMTreeNode *parent, WMTreeNode *aNode) {
    return WMInsertNodeInTree(parent, -1, aNode);
}

#undef WMGetFirstInTree
/* Returns first tree node that has data == cdata */
WMTreeNode* WMGetFirstInTree(WMTreeNode *aTree, void *cdata) {
    return WMFindInTree(aTree, NULL, cdata);
}

#undef WMFlushConnection
int WMFlushConnection(WMConnection *cPtr) {
    return WMSendConnectionData(cPtr, NULL);
}

#undef WMGetConnectionQueuedData
WMArray* WMGetConnectionQueuedData(WMConnection *cPtr) {
    return WMGetConnectionUnsentData(cPtr);
}

#undef WMWidgetClass
W_Class WMWidgetClass(WMWidget *widget) {
    return (((W_WidgetType*)(widget))->widgetClass);
}

#undef WMWidgetView
WMView* WMWidgetView(WMWidget *widget) {
    return (((W_WidgetType*)(widget))->view);
}

#undef WMCreateCommandButton
WMButton* WMCreateCommandButton(WMWidget *parent) {
    return WMCreateCustomButton(parent, WBBSpringLoadedMask|WBBPushInMask
                                |WBBPushLightMask|WBBPushChangeMask);
}

#undef WMCreateRadioButton
WMButton* WMCreateRadioButton(WMWidget *parent) {
    return WMCreateButton(parent, WBTRadio);
}

#undef WMCreateSwitchButton
WMButton* WMCreateSwitchButton(WMWidget *parent) {
    return WMCreateButton(parent, WBTSwitch);
}

#undef WMAddListItem
WMListItem* WMAddListItem(WMList *lPtr, char *text)
{
    return WMInsertListItem(lPtr, -1, text);
}

#undef WMCreateText
WMText* WMCreateText(WMWidget *parent) {
    return WMCreateTextForDocumentType(parent, NULL, NULL);
}

#undef WMRefreshText
void WMRefreshText(WMText *tPtr) {
    return WMThawText(tPtr);
}

#undef WMClearText
void WMClearText(WMText *tPtr) {
    return WMAppendTextStream(tPtr, NULL);
}
%}


