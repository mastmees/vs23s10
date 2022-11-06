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

import sys
from tkinter import *
from tkinter import messagebox
from tkinter import filedialog
from copy import copy
from simpledialog import Dialog
import random
import struct
import os
import json
import PIL.Image,PIL.ImageTk,PIL.ImageDraw

class EntryDialog(Dialog):

  def body(self,master,parameters):
    prompt,value=parameters
    Label(master,text=prompt).grid(row=0,sticky=W)
    self.value=StringVar()
    self.value.set(value)
    self.entry=Entry(master,textvariable=self.value)
    self.entry.grid(row=1,sticky=W)
    self.result=None
    #self.bind("<key>",self.key)
    return self.entry

  def apply(self):
    self.result=self.value.get()

  def validate(self):
    result=self.value.get()
    return 1
    

class App(Tk):

  def __init__(self,*args,**kwargs):
    Tk.__init__(self, *args, **kwargs)
    #self.geometry("800x600")
    self.title("GUI Font Editor")
    buttonframe=Frame(self,relief=FLAT)
    buttonframe.grid(row=0,column=0,sticky=W+N)
    button=Button(buttonframe,text="Load",command=self.loadfile)
    button.grid(row=0,column=0,sticky=W+N)
    button=Button(buttonframe,text="Save",command=self.savefile)
    button.grid(row=0,column=1,sticky=W+N)
    button=Button(buttonframe,text="Save as...",command=self.savefile)
    button.grid(row=0,column=2,sticky=W+N)
    button=Button(buttonframe,text="Import",command=self.importfile)
    button.grid(row=0,column=3,sticky=W+N)
    button=Button(buttonframe,text="New",command=self.newfile)
    button.grid(row=0,column=4,sticky=W+N)
    button=Button(buttonframe,text="Export",command=self.exportfile)
    button.grid(row=0,column=5,sticky=W+N)
    button=Button(buttonframe,text="Presentation",command=self.renderfullfont)
    button.grid(row=0,column=6,sticky=W+N)
    button=Button(buttonframe,text="Export for OLED",command=self.exportoled)
    button.grid(row=0,column=7,sticky=W+N)
    
    self.stride=13
    self.sampletexts=[]
    if os.path.exists("samples.txt"):
      for l in open("samples.txt","r").readlines():
        l=l.strip()
        if l:
          self.sampletexts.append(l)
    if not self.sampletexts:
      self.sampletexts=[
        "QUICK BROWN FOX JUMPED OVER THE LAZY DOGS BACK.",
        "quick brown fox jumped over the lazy dogs back.",
        "!\"#%&/()=?_{[]}\\*+-01234567890"
      ]
    self.sampleindex=0
    cellframe=Frame(self,relief=FLAT)
    cellframe.grid(row=1,column=0,sticky=W+N)
    self.canvas=Canvas(cellframe,width=32*self.stride+2,height=32*self.stride+2, \
      borderwidth=0, highlightthickness=0,relief=FLAT)
    self.canvas.grid(row=0,column=0,sticky=W+N+E+S)
    self.statustext=StringVar()
    label=Label(cellframe,textvariable=self.statustext)
    label.grid(row=1,column=0)
    self.sampleframe=Frame(self,relief=SUNKEN)
    self.sampleframe.grid(row=2,column=0,sticky=N+W,columnspan=3)
    #
    self.image=PIL.Image.new("RGBA",(800,36))
    self.draw=PIL.ImageDraw.Draw(self.image)
    self.sampleimage=PIL.ImageTk.PhotoImage(self.image)
    self.sample=Label(self.sampleframe,image=self.sampleimage)
    self.sample.grid(row=0,column=0,sticky=N+W)
    self.canvas.bind("<Button-1>", self.canvas_click)
    self.bind("<Key>",self.key)
    
    infoframe=Frame(self,relief=FLAT)
    infoframe.grid(row=1,column=1,sticky=W+N)
    self.charcode_info=StringVar()
    self.char_info=StringVar()
    self.width_info=StringVar()
    self.height_info=StringVar()
    label=Label(infoframe,textvariable=self.width_info)
    label.grid(row=4,column=0,sticky=W+N)
    label=Label(infoframe,textvariable=self.height_info)
    label.grid(row=5,column=0,sticky=W+N)
    label=Label(infoframe,textvariable=self.charcode_info)
    label.grid(row=6,column=0,sticky=W+N)
    label=Label(infoframe,textvariable=self.char_info)
    label.grid(row=7,column=0,sticky=W+N)
    self.preview=PhotoImage(width=32, height=32)
    label=Label(infoframe,image=self.preview)
    label.grid(row=0,column=0,sticky=N+W)
    label=Label(infoframe,text="\t")
    label.grid(row=8,column=0,sticky=W+N)
    guideframe=Frame(self,relief=SUNKEN)
    guideframe.grid(row=1,column=2,sticky=W+N)
    self.guide=Listbox(guideframe,width=50,height=30,bg=self.cget("bg"),font=("Courier",10),relief=FLAT)
    self.guide.grid(row=0,column=0,sticky=W+N+E+S)
    for l in open("guide.txt").readlines():
      self.guide.insert(END,l.strip())    
    self.clip=[]
    self.cx=0
    self.cy=0
    self.newfile()
    self.load_state()

  def askvalue(self,prompt,currentvalue):
    d=EntryDialog(self,parameters=(prompt,currentvalue))
    return d.result

  def newfile(self):
    self.cell=[]
    self.font=[]
    self.widths=[]
    for i in range(32):
      self.cell.append(0)
    for i in range(256):
      self.font.append(list(self.cell))
      self.widths.append(8)
    self.fontheight=16
    self.markchar=None
    self.currentchar=None
    self.filename=None
    self.changeto(ord("A"))
    
  def loadfile(self,filename=None):
    self.currentchar=None
    if filename==None:
      filename = filedialog.askopenfilename(initialdir = ".",\
        title = "Select file",filetypes = (("font files","*.vfnt"),("all files","*.*")))
    if filename and os.path.exists(filename):
      filename=os.path.abspath(filename)
      self.status(filename)
      content=open(filename,"rb").read()
      if len(content)==256*2+6+256*32*4 and content[0:4]==b'FONT':
        self.fontheight=struct.unpack(">H",content[4:6])[0]
        for i in range(256):
          self.widths[i]=struct.unpack(">H",content[i*2+6:i*2+8])[0]
        for i in range(256):
          l=list()
          c=content[i*32*4+518:i*32*4+518+32*4]
          for j in range(32):
            l.append(struct.unpack(">I",c[j*4:j*4+4])[0])
          self.font[i]=l
        self.filename=filename
    self.changeto(ord("A"))
  
  def savefileas(self):
    ofn=self.filename
    self.filename=None
    self.savefile()
    if self.filename==None:
      self.filename=ofn
    
  def savefile(self):
    self.changeto(self.currentchar)
    if self.filename==None:
      self.filename = filedialog.asksaveasfilename(initialdir = ".",\
        title = "Save as...",filetypes = (("font files","*.vfnt"),("all files","*.*")))
    if self.filename:
      self.status(self.filename)
      content=bytearray(b'FONT')
      content.extend(struct.pack(">H",self.fontheight))
      for h in self.widths:
        content.extend(struct.pack(">H",h))
      for h in self.font:
        for i in h:
          content.extend(struct.pack(">I",i))
      with open(self.filename,"wb") as f:
        f.write(content)
      
  # import simple 8 pixel wide fixed font, the file is assumed to have
  # 256 character definitions, so the character height is simply file size
  # divided by 256
  #    
  def importfile(self):
    self.currentchar=None
    filename = filedialog.askopenfilename(initialdir = ".",\
      title = "Select file",filetypes = (("font files","*.fnt"),("all files","*.*")))
    if filename:
      self.status(filename)
      d=open(filename,"rb").read()
      h=int(len(d)/256)
      if h<7 or h>32:
        self.status("Character height must be 7..32")
        return
      self.fontheight=h
      i=0
      for c in range(256):
        n=list()
        for l in range(h):
          n.append(d[i]<<24)
          i+=1
        while len(n)<32:
          n.append(0)
        self.font[c]=n
        self.widths[c]=8
      self.filename=None
    self.changeto(ord("A"))

  def exportfile(self):
    self.changeto(self.currentchar)
    b=self.first_defined()
    if b==1 or b==33:
      b-=1
    e=self.last_defined()
    if b==None or e==None:
      self.status("No defined chars")
      return
    name=os.path.basename(self.filename).partition(".")[0]
    if self.is_variable_font():
      widths="uint8_t const %s_widths[] PROGMEM = {\n"%name
      for i in range(b,e+1):
        widths+="  %d,\n"%self.widths[i]
      widths+="};\n\n"
      
      bitmaps=""
      for c in range(b,e+1):
        bitmaps+="uint8_t const %s_bitmaps_%d[] PROGMEM = {\n  "%(name,c)
        bw=int((self.widths[c]+7)/8)
        for d in self.font[c][0:self.fontheight]:
          for i in range(bw):
            bitmaps+="0x%02x,"%(((d<<(8*i))>>24)&0xff)
          bitmaps+="\n  "
        bitmaps+="\n};\n"
      
      font="const FONT %s_font = {\n"%name
      font+=" %d,%d,\n"%(b,e)
      font+=" %d,0,\n"%(self.fontheight)
      font+=" %s_widths,\n"%name
      font+=" {\n"
      for i in range(b,e+1):
        font+="  %s_bitmaps_%d,\n"%(name,i)
      font+=" }\n"
      font+="};\n"
    else:
      widths=""
      bitmaps="uint8_t const %s_bitmaps[] PROGMEM = {\n  "%name
      for c in range(b,e+1):
        bw=int((self.widths[c]+7)/8)
        bitmaps+="// 0x%02x\n  "%c
        for d in self.font[c][0:self.fontheight]:
          for i in range(bw):
            bitmaps+="0x%02x,"%(((d<<(8*i))>>24)&0xff)
          bitmaps+="\n  "
      bitmaps+="\n};\n"
              
      font="const FONT %s_font = {\n"%name
      font+=" %d,%d,\n"%(b,e)
      font+=" %d,%d,\n"%(self.fontheight,self.widths[0])
      font+=" NULL,\n"
      font+=" {%s_bitmaps}\n"%(name)
      font+="};\n"
    with open("%s.c"%self.filename,"wt") as f:
      f.write('#include "font.h"\n\n')
      f.write(bitmaps)
      f.write(widths)
      f.write(font)

  # OLED fonts are set up in columns of 8 pixels
  # for effective rendering. font heights in
  # multiples of 8 pixels will work, ie. 3 pixel wide
  # and 16 pixel high char is made up of 6 bytes
  #
  # 0 1 2
  # 0 1 2
  # 0 1 2
  # 0 1 2
  # 0 1 2
  # 0 1 2
  # 0 1 2
  # 0 1 2
  # 3 4 5
  # 3 4 5
  # 3 4 5
  # 3 4 5
  # 3 4 5
  # 3 4 5
  # 3 4 5
  # 3 4 5
  #
   
  def exportoled(self):
    self.changeto(self.currentchar)
    if self.fontheight%8:
      messagebox.showerror(title="Error",message="Font height must be multiple of 8 for OLED")
      return
    begin=self.first_defined()
    if begin==1 or begin==33:
      begin-=1
    end=self.last_defined()
    if begin==None or end==None:
      self.status("No defined chars")
      return
    name=os.path.basename(self.filename).partition(".")[0]
    is_var=self.is_variable_font()
    if is_var:
      widths="uint8_t const %s_widths[] PROGMEM = {\n"%name
      for i in range(begin,end+1):
        widths+="  %d,\n"%self.widths[i]
      widths+="};\n\n"
      bitmaps=""
    else:
      widths=""
      bitmaps="uint8_t const %s_bitmaps[] PROGMEM = {\n  "%name

    for c in range(begin,end+1):
      if is_var:
        bitmaps+="uint8_t const %s_bitmaps_%d[] PROGMEM = {\n  "%(name,c)
      for y in range(self.fontheight>>3):
        block=list(self.font[c][y<<3:(y+1)<<3])
        for x in range(self.widths[c]):
          b=0
          for i in range(len(block)):
            if block[i]&0x80000000:
              b=b|(1<<i)
            block[i]<<=1
          bitmaps+="0x%02x,"%(b)
        bitmaps+="\n  "
      if is_var:
        bitmaps+="\n};\n"
    
    if not is_var:
      bitmaps+="\n};\n"
        
    font="const FONT %s_font = {\n"%name
    font+=" %d,%d,\n"%(begin,end)
    if is_var:
      font+=" %d,0,\n"%(self.fontheight)
      font+=" %s_widths,\n"%name
      font+=" {\n"
      for i in range(begin,end+1):
        font+="  %s_bitmaps_%d,\n"%(name,i)
      font+=" }\n"
      font+="};\n"
    else:  
      font+=" %d,%d,\n"%(self.fontheight,self.widths[0])
      font+=" NULL,\n"
      font+=" {%s_bitmaps}\n"%(name)
      font+="};\n"
    with open("%s.oled.c"%self.filename,"wt") as f:
      f.write('#include "font.h"\n\n')
      f.write(bitmaps)
      f.write(widths)
      f.write(font)
      
  def changeto(self,charnum):
    if self.currentchar!=None:
      self.font[self.currentchar]=list(self.cell)
    self.currentchar=charnum
    self.cell=list(self.font[self.currentchar])
    self.charcode_info.set("c:%d"%charnum)
    self.char_info.set("%s"%chr(charnum))
    self.after(1,self.redraw)
    
  def getpixel(self,x,y):
    bit=0x80000000>>x
    if self.cell[y]&bit:
      return 1
    return 0
  
  def setpixel(self,x,y,value):
    bit=0x80000000>>x
    if value:
      self.cell[y]|=bit
    else:
      self.cell[y]&=~bit
      
  def redraw(self):
    self.canvas.delete(ALL)
    for y in range(31,-1,-1):
      for x in range(31,-1,-1):
        if y>=self.fontheight or x>=self.widths[self.currentchar]:
          border="lightgrey"
        else:
          border="grey"
        if self.getpixel(x,y):
          inside="black"
          self.preview.put("#ffffff",to=(x,y,x+1,y+1))
        else:
          inside="lightgrey"
          self.preview.put("#000000",to=(x,y,x+1,y+1))
        if y==self.cy and x==self.cx:
          continue
        self.canvas.create_rectangle(x*self.stride,y*self.stride,\
            x*self.stride+self.stride,y*self.stride+self.stride,\
            fill=inside,outline=border)
    border="white"
    if self.getpixel(self.cx,self.cy):
      inside="black"
    else:
      inside="lightgrey" 
    self.canvas.create_rectangle(self.cx*self.stride,self.cy*self.stride,\
        self.cx*self.stride+self.stride,self.cy*self.stride+self.stride,\
        fill=inside,outline=border)
    self.width_info.set("w:%d"%self.widths[self.currentchar])
    self.height_info.set("h:%d"%self.fontheight)

  def status(self,message):
    self.statustext.set("%s"%message)

  def canvas_click(self,event):
    canvas = event.widget
    x = canvas.canvasx(event.x)
    y = canvas.canvasy(event.y)
    self.cy=int(y/self.stride)
    self.cx=int(x/self.stride)
    self.setpixel(self.cx,self.cy,not self.getpixel(self.cx,self.cy))
    self.after(1,self.redraw)
    
  def rendersample(self,text,black=(0,0,0),white=(255,255,255)):
    self.changeto(self.currentchar)
    self.draw.rectangle((0,0,self.image.width,self.image.height),black)
    x=0
    for c in text:
      c=ord(c)
      if c>255:
        continue
      if x+self.widths[c]>=self.image.width:
        break
      for cy in range(32):
        d=self.font[c][cy]
        for cx in range(self.widths[c]):
          if d&0x80000000:
            self.draw.point((x+cx,cy+2),white)
          d<<=1
      x+=self.widths[c]
    self.sampleimage=PIL.ImageTk.PhotoImage(self.image)
    self.sample.configure(image=self.sampleimage)
  
  def renderfullfont(self):
  
    def renderchar(draw,c,x,y,psize,background=(255,255,255)):
      draw.rectangle((x,y,x+self.widths[c]*psize,y+self.fontheight*psize),background)
      for cy in range(self.fontheight):
        d=self.font[c][cy]
        for cx in range(self.widths[c]):
          if d&0x80000000:
            draw.rectangle((x+cx*psize,y+cy*psize,x+cx*psize+psize,y+cy*psize+psize),(0,0,0))
          d<<=1
    
    def rendertext(draw,text,x,y,w,h,scale=4):
      cx=0
      cy=0
      for c in text:
        cc=ord(c)
        if cx+self.widths[cc]>=w or cc==10:
          if cy+self.fontheight>=h:
            break
          cy+=self.fontheight*scale
          cx=0
        if cc!=10:
          renderchar(draw,cc,x+cx,y+cy,scale)
          cx+=self.widths[cc]*scale
    
    def textwidth(text):
      w=0
      for c in text:
        w+=self.widths[ord(c)]
      return w
        
    img=PIL.Image.new("RGBA",(11*300,16*300)) # A4 paper size at 300 dpi
    draw=PIL.ImageDraw.Draw(img)
    draw.rectangle((0,0,img.width,img.height),(255,255,255))
    # large grid for 16x16 characters
    # use 3000x3000 square area for this, pad each character by equivalent of a pixel on each side
    #print("max char width",max(self.widths))
    gs=max(max(self.widths),self.fontheight)+2
    #print("font grid size",gs)
    s=int(3000/(gs*16))
    xo=int((img.width-(gs*s*16))/2)
    #print("pixel cell size",s)
    titlescale=int(150/self.fontheight)+1
    th=self.fontheight*titlescale
    t=os.path.basename(self.filename).rpartition(".")[0]
    rendertext(draw,t,(img.width-textwidth(t)*titlescale)>>1,xo,img.width>>1,self.fontheight,scale=titlescale)
    for y in range(16):
      for x in range(16):
        cc=y*16+x
        cxo=int((gs-self.widths[cc])/2*s)
        renderchar(draw,cc,cxo+xo+x*s*gs,xo*2+th+y*s*gs,s,(240,240,240))
    #
    if os.path.exists("presentation.txt"):
      text=open("presentation.txt","r").read()
      rendertext(draw,text,xo,xo*3+th+gs*s*16,gs*s*16,img.height-xo*4-gs*s*16-th,scale=3)
    n=os.path.basename(self.filename).rpartition(".")[0]+".png"
    fn=os.path.join(os.path.dirname(self.filename),n)
    img.save(fn,"PNG")
    os.system('open "'+fn+'"')
    
  def key(self,event):
    #for a in ('char', 'delta', 'height', 'keycode', 'keysym', 'keysym_num', 'num', 'send_event', 'serial', 'state', 'time',
    #  'type', 'widget', 'width', 'x', 'x_root', 'y', 'y_root'):
    #  t=getattr(event,a)
    #  if type(t) is int:
    #    t="0x%x"%t
    #  print(a,"=",str(t))
    if event.keysym=="a":
      t=self.askvalue("Enter new sample text","")
      if t:
        self.sampletexts.append(t)
        self.sampleindex=len(self.sampletexts)-1
        self.rendersample(t)
    elif event.keysym=="q":
      self.savefile()
      self.quit()
    elif event.keysym=='Q':
      self.quit()
    elif event.keysym=='Right': #right
      self.cx=(self.cx+1)%32
    elif event.keysym=='Down': #down
      self.cy=(self.cy+1)%32
    elif event.keysym=='Up': #up
      self.cy=(self.cy-1)%32
    elif event.keysym=='Left': #left
      self.cx=(self.cx-1)%32
    elif event.keysym in ('Next','>'): #pgdn
      self.changeto((self.currentchar+1)%256)
    elif event.keysym in ('Prior','<'): #pgup
      self.changeto((self.currentchar-1)%256)
    elif event.keysym=='i':
      if self.markchar==None:
        for i in range(32):
          self.cell[i]^=0xffffffff
      else:
        self.changeto(self.currentchar)
        if self.markchar<self.currentchar:
          b=self.markchar
          e=self.currentchar
        else:
          b=self.currentchar
          e=self.markchar
        for i in range(b,e+1):
          for r in range(32):
            self.font[i][r]=self.font[i][r]^0xffffffff
        self.status("Inverted %d characters"%(e+1-b))
        b=self.currentchar
        self.currentchar=None
        self.changeto(b)
        self.markchar=None
    elif event.keysym=='s':
      self.rendersample(self.sampletexts[self.sampleindex])
      self.sampleindex=(self.sampleindex+1)%len(self.sampletexts)
    elif event.keysym=='z':
      for i in range(32):
        self.cell[i]=0
    elif event.keysym=='m':
      self.markchar=self.currentchar
      self.status("Mark placed")
    elif event.keysym=='c':
      self.changeto(self.currentchar)
      self.clip=[]
      if self.markchar==None:
        self.markchar=self.currentchar
      if self.markchar<self.currentchar:
        b=self.markchar
        e=self.currentchar
      else:
        b=self.currentchar
        e=self.markchar
      for i in range(b,e+1):
        self.clip.append(list(self.font[i]))
      self.status("Copied %d characters"%len(self.clip))
      self.markchar=None
    elif event.keysym=='p':
      b=self.currentchar
      for e in self.clip:
        self.font[b]=e
        b=(b+1)%256
      b=self.currentchar
      self.currentchar=None
      self.changeto(b)
      self.status("Pasted %d characters"%len(self.clip))
    elif event.keysym=='D':
      for i in range(15,-1,-1):
        d=self.cell[i]
        o=0
        for j in range(16):
          if d&(0x80000000>>j):
            o|=(0xc0000000>>j*2)
        self.cell[i*2+1]=o
        self.cell[i*2]=o
    elif event.keysym=='u':
      self.cell.append(self.cell.pop(0))
    elif event.keysym=='d':
      self.cell.insert(0,self.cell.pop())
    elif event.keysym=='l':
      for i in range(32):
        d=self.cell[i]&0x80000000
        self.cell[i]<<=1
        if d:
          self.cell[i]|=1
    elif event.keysym=='r':
      for i in range(32):
        d=self.cell[i]&1
        self.cell[i]>>=1
        if d:
          self.cell[i]|=0x80000000
    elif event.keysym=='T':
      if self.fontheight<32:
        self.fontheight+=1
    elif event.keysym=='S':
      if self.fontheight>1:
        self.fontheight-=1
    elif event.keysym=='W':
      if self.widths[self.currentchar]<32:
        self.widths[self.currentchar]+=1
      self.status("Width = %d"%self.widths[self.currentchar])
    elif event.keysym=='N':
      if self.widths[self.currentchar]>1:
        self.widths[self.currentchar]-=1
      self.status("Width = %d"%self.widths[self.currentchar])
    elif event.keysym=='space':
      self.setpixel(self.cx,self.cy,not self.getpixel(self.cx,self.cy))
    else:
      self.status("Unhandled key %s"%(event.keysym))
    self.after(1,self.redraw)

  def load_state(self):
    if not os.path.exists(".gfe.state"):
      return
    for l in open(".gfe.state","rt").readlines():
      p=l.partition("=")
      if p[1]=="=":
        k=p[0].strip().lower()
        v=p[2].partition("#")[0].strip()
        if k=="file":
          self.loadfile(v)
        elif k=="char":
          self.changeto(int(v))
        elif k=="cx":
          self.cx=int(v)
        elif k=="cy":
          self.cy=int(v)
        elif k=="mark":
          self.markchar=int(v)
        elif k=="clip":
          self.clip=json.loads(v)
  
  def save_state(self):
    with open(".gfe.state","wt") as f:
      if not self.filename:
        filename=""
      else:
        filename=self.filename
      f.write("file=%s\n"%filename)
      f.write("char=%d\n"%self.currentchar)
      f.write("cx=%d\n"%self.cx)
      f.write("cy=%d\n"%self.cy)
      if self.markchar:
        f.write("mark=%d\n"%self.markchar);
      f.write("clip=%s\n"%json.dumps(self.clip))
  
  def is_variable_font(self):
    w=self.widths[0]
    for i in self.widths[1:]:
      if i!=w:
        return True
    return False

  def empty_char(self,charnum):
    for i in self.font[charnum]:
      if i!=0:
        return False
    return True
    
  def first_defined(self):
    for i in range(256):
      if not self.empty_char(i):
        return i
    return None
  
  def last_defined(self):
    for i in range(255,-1,-1):
      if not self.empty_char(i):
        return i
    return None
      
app=App()
if len(sys.argv)>1:
  app.loadfile(sys.argv[1])
app.mainloop()
app.save_state()
