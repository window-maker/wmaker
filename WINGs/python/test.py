#!/usr/bin/env python

import sys
from WINGs import *

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

