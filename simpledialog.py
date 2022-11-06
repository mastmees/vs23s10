#
# The MIT License (MIT)
#
# Copyright (c) 2022 Madis Kaal <mast@nomad.ee>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
from tkinter import *

class Dialog(Toplevel):

  def __init__(self, parent, title = None, parameters = None, cancelbutton = True):
    Toplevel.__init__(self, parent)
    self.transient(parent)
    if title:
      self.title(title)
    self.parent = parent
    self.result = None
    body = Frame(self)
    self.initial_focus = self.body(body,parameters)
    body.grid(padx=5, pady=5)
    self.buttonbox(cancelbutton)
    self.wait_visibility()
    self.grab_set()
    if not self.initial_focus:
      self.initial_focus = self
    self.protocol("WM_DELETE_WINDOW", self.cancel)
    self.geometry("+%d+%d" % (parent.winfo_rootx()+50,
                  parent.winfo_rooty()+50))
    self.initial_focus.focus_set()
    self.wait_window(self)

  def body(self, master, parameters):
    # create dialog body.  return widget that should have
    # initial focus.  this method should be overridden
    pass

  def buttonbox(self,cancelbutton):
    # add standard button box. override if you don't want the
    # standard buttons
    box = Frame(self)
    w = Button(box, text="OK", width=10, command=self.ok, default=ACTIVE)
    w.grid(row=2,column=0,padx=5, pady=5)
    if cancelbutton:
      w = Button(box, text="Cancel", width=10, command=self.cancel)
      w.grid(row=2,column=1,padx=5, pady=5)
      self.bind("<Escape>", self.cancel)
    self.bind("<Return>", self.ok)
    box.grid()

  def ok(self, event=None):
    if not self.validate():
      self.initial_focus.focus_set() # put focus back
      return
    self.withdraw()
    self.update_idletasks()
    self.apply()
    self.cancel()

  def cancel(self, event=None):
    # put focus back to the parent window
    self.parent.focus_set()
    self.destroy()

  def validate(self):
    return 1 # override

  def apply(self):
    pass # override

