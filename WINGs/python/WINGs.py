#!/usr/bin/env python

import sys
import wings

# Some useful constants

False = 0
True = 1

# check about None as action for buttonAction/windowCloseAction ...

################################################################################
# Exceptions
################################################################################
from exceptions import Exception, StandardError

class Error(StandardError):
    pass

del Exception, StandardError

class WMTimer:
    def __init__(self, milliseconds, callback, cdata=None, persistent=False):
        if persistent:
            self._o = wings.pyWMAddPersistentTimerHandler(milliseconds, (callback, cdata))
        else:
            self._o = wings.pyWMAddTimerHandler(milliseconds, (callback, cdata))

    def __del__(self):
        wings.pyWMDeleteTimerHandler(self._o)
    #delete = __del__

class WMPersistentTimer(WMTimer):
    def __init__(self, milliseconds, callback, cdata=None):
        WMTimer.__init__(self, milliseconds, callback, cdata, persistent=True)


class WMScreen:
    __readonly = ('display', 'width', 'height', 'depth')

    def __init__(self, appname, display="", simpleapp=False):
        wings.WMInitializeApplication(appname, len(sys.argv), sys.argv)	
        self._o = wings.pyWMOpenScreen(display, simpleapp)
        if not self._o:
            raise Error, "Cannot open display %s" % display
        self.__dict__['display'] = wings.WMScreenDisplay(self._o)
        self.__dict__['width'] = wings.WMScreenWidth(self._o)
        self.__dict__['height'] = wings.WMScreenHeight(self._o)
        self.__dict__['depth'] = wings.WMScreenDepth(self._o)

    def __setattr__(self, name ,value):
        if name in self.__readonly:
            #raise AttributeError, "'%s' is a read-only WMScreen attribute" % name
            raise Error, "'%s' is a read-only WMScreen attribute" % name
        self.__dict__[name] = value

    def mainLoop(self):
        wings.pyWMScreenMainLoop(self._o)

    def breakMainLoop(self):
        wings.pyWMBreakScreenMainLoop(self._o)

    def runModalLoop(self, view):
        wings.pyWMRunModalLoop(self._o, view)

    def breakModalLoop(self):
        wings.WMBreakModalLoop(self._o)

    def size(self):
        return (self.width, self.height)


class WMView:
    pass


class WMWidget(WMView):
    def __init__(self):
        self._o = None
        if self.__class__ == WMWidget:
            raise Error, "a WMWidget can't be instantiated directly"

    def __del__(self):
        if (self._o != None):
            wings.WMDestroyWidget(self._o)
    
    def resize(self, width, height):
        wings.WMResizeWidget(self._o, width, height)

    def move(self, x, y):
        wings.WMMoveWidget(self._o, x, y)

    def realize(self):
        wings.WMRealizeWidget(self._o)

    def show(self):
        wings.WMMapWidget(self._o)

    def hide(self):
        wings.WMUnmapWidget(self._o)

    def redisplay(self):
        wings.WMRedisplayWidget(self._o)
    
    def width(self):
        return wings.WMWidgetWidth(self._o)
    
    def height(self):
        return wings.WMWidgetHeight(self._o)

    def screen(self):
        return wings.WMWidgetScreen(self._o)

    def view(self):
        return wings.WMWidgetView(self._o)

    def setFocusTo(self, other):
        wings.WMSetFocusToWidget(other._o)


class WMWindow(WMWidget):
    def __init__(self, screen, name, style=wings.WMTitledWindowMask
                 |wings.WMClosableWindowMask|wings.WMMiniaturizableWindowMask
                 |wings.WMResizableWindowMask):
        WMWidget.__init__(self)
        self._o = wings.WMCreateWindowWithStyle(screen._o, name, style)
    
    def setMinSize(self, minWidth, minHeight):
        wings.WMSetWindowMinSize(self._o, minWidth, minHeight)
    
    def setMaxSize(self, maxWidth, maxHeight):
        wings.WMSetWindowMaxSize(self._o, maxWidth, maxHeight)
    
    def setInitialPosition(self, x, y):
        wings.WMSetWindowInitialPosition(self._o, x, y)
    
    def setTitle(self, title):
        wings.WMSetWindowTitle(self._o, title)

    def setCloseAction(self, action, data=None):
        if action!=None and (not callable(action)):
            raise Error, "action needs to be a callable object or None"
        wings.pyWMSetWindowCloseAction(self._o, (self, action, data))


class WMPanel(WMWindow):
    def __init__(self, owner, name, style=wings.WMTitledWindowMask
                 |wings.WMClosableWindowMask|wings.WMResizableWindowMask):
        WMWidget.__init__(self)
        self._o = wings.WMCreatePanelWithStyleForWindow(owner._o, name, style)

class WMFrame(WMWidget):
    def __init__(self, parent, title=None):
        WMWidget.__init__(self)
        self._o = wings.WMCreateFrame(parent._o)
        self.setTitle(title)

    def setRelief(self, relief):
        wings.WMSetFrameRelief(self._o, relief)

    def setTitle(self, title=""):
        wings.WMSetFrameTitle(self._o, title)

    def setTitlePosition(self, position):
        wings.WMSetFrameTitlePosition(self._o, position)

class WMLabel(WMWidget):
    def __init__(self, parent, text=None):
        WMWidget.__init__(self)
        self._o = wings.WMCreateLabel(parent._o)
        self.setText(text)

    def setWraps(self, flag):
        # bool(flag) for python2.2
        wings.WMSetLabelWraps(self._o, flag)

    def setRelief(self, relief):
        wings.WMSetLabelRelief(self._o, relief)

    def setText(self, text=""):
        wings.WMSetLabelText(self._o, text)

    def setTextColor(self, color):
        wings.WMSetLabelTextColor(self._o, color)

    def setFont(self, font):
        wings.WMSetLabelFont(self._o, font)

    def setTextAlignment(self, alignment):
        wings.WMSetLabelTextAlignment(self._o, alignment)

    def setImage(self, image):
        wings.WMSetLabelImage(self._o, image)

    def setImagePosition(self, position):
        wings.WMSetLabelImagePosition(self._o, position)

    def text(self):
        return wings.WMGetLabelText(self._o)

    def font(self):
        return wings.WMGetLabelFont(self._o)

    def image(self):
        return wings.WMGetLabelImage(self._o)


class WMBox(WMWidget):
    def __init__(self, parent):
        WMWidget.__init__(self)
        self._o = wings.WMCreateBox(parent._o)

    def setHorizontal(self, flag):
        # bool(flag) for python2.2
        wings.WMSetBoxHorizontal(self._o, flag)

    def setBorderWidth(self, width):
        wings.WMSetBoxBorderWidth(self._o, width)

    def addSubview(self, view, expand, fill, minSize, maxSize, space):
        wings.WMAddBoxSubview(self._o, view, expand, fill, minSize, maxSixe, space)

    def addSubviewAtEnd(self, view, expand, fill, minSize, maxSize, space):
        wings.WMAddBoxSubviewAtEnd(self._o, view, expand, fill, minSize, maxSixe, space)

    def removeSubview(self, view):
        wings.WMRemoveBoxSubview(self._o, view)


class WMButton(WMWidget): # not for user instantiation
    def __init__(self, parent):
        WMWidget.__init__(self)
        if self.__class__ == WMButton:
            raise Error, "a WMButton can't be instantiated directly"


    def setText(self, text=""):
        wings.WMSetButtonText(self._o, text)

    def setAction(self, action, data=None):
        if action!=None and (not callable(action)):
            raise Error, "action needs to be a callable object or None"
        wings.pyWMSetButtonAction(self._o, (self, action, data))

    def performClick(self):
        wings.WMPerformButtonClick(self._o)

    def setEnabled(self, flag):
        # bool(flag) for python2.2
        wings.WMSetButtonEnabled(self._o, flag)

    def isEnabled(self):
        return wings.WMGetButtonEnabled(self._o)

    def setImageDimsWhenDisabled(self, flag):
        # bool(flag)
        wings.WMSetButtonImageDimsWhenDisabled(self._o, flag)
    
    def setImage(self, image):
        wings.WMSetButtonImage(self_.o, image)
    
    def setAlternateImage(self, image):
        wings.WMSetButtonAltImage(self._o, image)
    
    def setImagePosition(self, position):
        wings.WMSetButtonImagePosition(self._o, position)
    
    def setImageDefault(self):
        wings.WMSetButtonImageDefault(self._o)
    

class WMCommandButton(WMButton):
    def __init__(self, parent):
        WMButton.__init__(self, parent)
        self._o = wings.WMCreateCommandButton(parent._o)


class WMSwitchButton(WMButton):
    def __init__(self, parent):
        WMButton.__init__(self, parent)
        self._o = wings.WMCreateSwitchButton(parent._o)


class WMRadioButton(WMButton):
    def __init__(self, parent, group=None):
        WMButton.__init__(self, parent)
        self._o = wings.WMCreateRadioButton(parent._o)
        if group:
            wings.WMGroupButtons(group._o, self._o)


class WMListItem:
    pass


class WMList(WMWidget):
    def __init__(self, parent):
        WMWidget.__init__(self)
        self._o = wings.WMCreateList(parent._o)

    def allowEmptySelection(self, flag):
        # bool(flag)
        wings.WMSetListAllowEmptySelection(self._o, flag)

    def allowMultipleSelection(self, flag):
        # bool(flag)
        wings.WMSetListAllowMultipleSelection(self._o, flag)

    def addItem(self, item):
        wings.WMAddListItem(self._o, item)

    def insertItem(self, row, item):
        wings.WMInsertListItem(self._o, row, item)

    def sortItems(self):
        wings.WMSortListItems(self._o)

    def rowWithTitle(self, title):
        return wings.WMFindRowOfListItemWithTitle(self._o, title)

    def selectedItemRow(self):
        return wings.WMGetListSelectedItemRow(self._o)

    def selectedItem(self):
        return wings.WMGetListSelectedItem(self._o)

    def removeItem(self, index):
        wings.WMRemoveListItem(self._o, index)

    def selectItem(self, index):
        wings.WMSelectListItem(self._o, index)

    def unselectItem(self, index):
        wings.WMUnselectListItem(self._o, index)


class WMTextFieldDelegate:
    __callbacks = ('didBeginEditing', 'didChange', 'didEndEditing',
                   'shouldBeginEditing', 'shouldEndEditing')

    def __init__(self):
        self.__dict__['data'] = None
        self.__dict__['didBeginEditing'] = None
        self.__dict__['didChange'] = None
        self.__dict__['didEndEditing'] = None
        self.__dict__['shouldBeginEditing'] = None
        self.__dict__['shouldEndEditing'] = None
 
    def __setattr__(self, name ,value):
        if name in self.__callbacks and value!=None and (not callable(value)):
            #raise AttributeError, "%s.%s needs to be a callable object or None" % (self.__class__.__name__, name)
            raise Error, "%s.%s needs to be a callable object or None" % (self.__class__.__name__, name)
        else:
            self.__dict__[name] = value


class WMTextField(WMWidget):
    def __init__(self, parent, text=""):
        WMWidget.__init__(self)
        self._o = wings.WMCreateTextField(parent._o)
        wings.WMSetTextFieldText(self._o, text)

    def setDelegate(self, delegate):
        if delegate.__class__ != WMTextFieldDelegate:
            raise Error, "textfield delegate must be of type 'WMTextFieldDelegate'"
        wings.pyWMSetTextFieldDelegate(self._o, (self, delegate))

    def delegate(self):
        return wings.pyWMGetTextFieldDelegate(self._o)

    def text(self):
        return wings.WMGetTextFieldText(self._o)

    def setEditable(self, flag):
        # bool(flag)
        wings.WMSetTextFieldEditable(self._o, flag)

    def setBordered(self, flag):
        # bool(flag)
        wings.WMSetTextFieldBordered(self._o, flag)

    def setBeveled(self, flag):
        # bool(flag)
        wings.WMSetTextFieldBeveled(self._o, flag)

    def setSecure(self, flag):
        # bool(flag)
        wings.WMSetTextFieldSecure(self._o, flag)

    def setCursorPosition(self, position):
        wings.WMSetTextFieldCursorPosition(self._o, position)

    def setNextText(self, next):
        wings.WMSetTextFieldNextTextField(self._o, next._o)

    def setPreviousText(self, previous):
        wings.WMSetTextFieldPrevTextField(self._o, previous._o)

    def setTextAlignment(self, alignment):
        wings.WMSetTextFieldAlignment(self._o, alignment)

    def isEditable(self):
        return wings.WMGetTextFieldEditable(self._o)

    def insertText(self, text, position):
        wings.WMInsertTextFieldText(self._o, text, position)

    def deleteText(self, start, count):
        wings.WMDeleteTextFieldRange(self._o, wings.wmkrange(start, count))

    def selectText(self, start, count):
        wings.WMSelectTextFieldRange(self._o, wings.wmkrange(start, count))

    def setFont(self, font):
        wings.WMSetTextFieldFont(self._o, font)

    def font(self):
        return wings.WMGetTextFieldFont(self._o)


################################################################################
# wrap the WINGs constants so we don't need wings.constant_name
################################################################################

# WMWindow title style
WMTitledWindowMask         = wings.WMTitledWindowMask
WMClosableWindowMask       = wings.WMClosableWindowMask
WMMiniaturizableWindowMask = wings.WMMiniaturizableWindowMask
WMResizableWindowMask      = wings.WMResizableWindowMask

# WMFrame title positions
WTPNoTitle     = wings.WTPNoTitle
WTPAboveTop    = wings.WTPAboveTop
WTPAtTop       = wings.WTPAtTop
WTPBelowTop    = wings.WTPBelowTop
WTPAboveBottom = wings.WTPAboveBottom
WTPAtBottom    = wings.WTPAtBottom
WTPBelowBottom = wings.WTPBelowBottom

# Alingments
WALeft      = wings.WALeft
WACenter    = wings.WACenter
WARight     = wings.WARight
WAJustified = wings.WAJustified	# not valid for textfields

# Image positions
WIPNoImage   = wings.WIPNoImage
WIPImageOnly = wings.WIPImageOnly
WIPLeft      = wings.WIPLeft
WIPRight     = wings.WIPRight
WIPBelow     = wings.WIPBelow
WIPAbove     = wings.WIPAbove
WIPOverlaps  = wings.WIPOverlaps

# Relief types
WRFlat   = wings.WRFlat
WRSimple = wings.WRSimple
WRRaised = wings.WRRaised
WRSunken = wings.WRSunken
WRGroove = wings.WRGroove
WRRidge  = wings.WRRidge
WRPushed = wings.WRPushed


# TextField events
WMReturnTextMovement  = wings.WMReturnTextMovement
WMEscapeTextMovement  = wings.WMEscapeTextMovement
WMIllegalTextMovement = wings.WMIllegalTextMovement
WMTabTextMovement     = wings.WMTabTextMovement
WMBacktabTextMovement = wings.WMBacktabTextMovement
WMLeftTextMovement    = wings.WMLeftTextMovement
WMRightTextMovement   = wings.WMRightTextMovement
WMUpTextMovement      = wings.WMUpTextMovement
WMDownTextMovement    = wings.WMDownTextMovement


if __name__ == "__main__":
    def quit(obj, data):
        #sys.exit()
        scr.breakMainLoop()

    def click(btn, list):
        print win.width(), win.height()
        print list.selectedItemRow()
        win2.show()
        scr.runModalLoop(win2.view())
        print txt2.text()

    def sayhi(btn, data):
        print "hi"

    def breakLoop(btn, data):
        #sys.exit()
        scr.breakModalLoop()
        win2.hide()

    def dc(object, data, action):
        print "didChange:", object, data, action

    def dbe(object, data, action):
        print "didBeginEditing:", object, data, action

    def dee(object, data, action):
        if action == wings.WMReturnTextMovement:
            if object == txt:
                object.setFocusTo(txt2)
            else:
                object.setFocusTo(txt)
        print "didEndEditing:", object, data, action, object.text()

    def tcb(one):
        old = list.selectedItemRow()
        list.selectItem(list.index)
        list.unselectItem(old)
        list.index = (list.index+1) % 3
        #print one

    scr = WMScreen("foobar")
    win = WMWindow(scr, "aWindow")
    win.setCloseAction(quit)
    win.setTitle("test window")
    win.resize(400, 180)
    win.setInitialPosition((scr.width-win.width())/2, (scr.height-win.height())/2)

    btn = WMCommandButton(win)
    btn.setText("Click Me")
    btn.resize(100, 25)
    btn.move(20, 20)
    btn.show()

    sw = WMSwitchButton(win)
    sw.setText("Some option")
    sw.resize(100, 25)
    sw.move(20, 50)
    sw.show()

    radios = []
    r = None
    j = 0
    for i in ["One", "Two", "Four"]:
        r = WMRadioButton(win, r)
        radios.append(r)
        r.show()
        r.setText(i)
        r.move(20, 70+j*25)
        r.resize(100, 25)
        j=j+1

    sw.setAction(sayhi)

    list = WMList(win)
    list.resize(100,100)
    list.move(130, 20)
    list.addItem("one")
    list.addItem("two")
    list.addItem("three")
    list.allowMultipleSelection(1)
    list.show()
    list.index = 0

    txtdel = WMTextFieldDelegate()
    txtdel.data = 'mydata'
    txtdel.didBeginEditing = dbe
    txtdel.didEndEditing = dee
    txtdel.didChange = dc

    txt = WMTextField(win, "abc")
    txt.resize(95, 20)
    txt.move(295, 20)
    txt.setDelegate(txtdel)
    txt.show()
    txt2 = WMTextField(win, "01234567890")
    txt2.resize(95, 20)
    txt2.move(295, 45)
    txt2.setDelegate(txtdel)
    txt2.show()

    txt.setNextText(txt2)
    txt2.setNextText(txt)

    label = WMLabel(win, "Text1:")
    label.setTextAlignment(WARight)
    label.move(240, 20)
    label.resize(55, 20)
    label.show()

    label2 = WMLabel(win, "mytext2:")
    label2.setTextAlignment(WARight)
    label2.move(240, 45)
    label2.resize(55, 20)
    label2.show()
    
    btn.setAction(click, list)
    
    frame = WMFrame(win, "My Frame")
    frame.resize(150, 50)
    frame.move(240, 70)
    #frame.setRelief(WRPushed)
    frame.show()
    
    ebtn = WMCommandButton(win)
    ebtn.setText("Exit")
    ebtn.resize(100, 25)
    ebtn.move(290, 147)
    ebtn.setAction(quit)
    ebtn.show()

    win.realize()
    win.show()

    timer = WMPersistentTimer(1000, tcb, win)
    #del(timer)
    #timer.delete()

    win2 = WMPanel(win, "anotherWindow", WMTitledWindowMask)
    win2.setTitle("transient test window")
    win2.resize(150, 50)
    win2.setInitialPosition((scr.width-win2.width())/2, (scr.height-win2.height())/2)

    btn7 = WMCommandButton(win2)
    btn7.setText("Dismiss")
    btn7.resize(100, 25)
    btn7.move(27, 10)
    btn7.show()
    btn7.setAction(breakLoop)

    win2.realize()
    
    scr.mainLoop()

