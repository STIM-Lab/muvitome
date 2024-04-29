import mvAlign as mva
import mvCorrect as mvc
import mvMosaic as mvm
import mvFlatfield as mvf
import mvTiffConvert as mvt
import tkinter as tk
from tkinter import filedialog as fd
import os
import skimage
import numpy as np

source_directory = "[Source Directory Not Selected]"
dest_directory = "[Destination Directory Not Selected]"
align_directory = "[Alignment Stack Not Selected]"
y_overlap = 0
x_overlap = 5
bool_flatfield = False

def set_source():
    global source_directory
    source_directory = fd.askdirectory()
    lbl_source["text"] = source_directory
    lbl_source["fg"] = "black"
    
def set_dest():
    global dest_directory
    dest_directory = fd.askdirectory()
    lbl_dest["text"] = dest_directory
    lbl_dest["fg"] = "black"
    
def set_flatfield():
    global bool_flatfield
    bool_flatfield = True
    
    
def begin_processing():
    global source_directory
    global dest_directory
   
    if bool_flatfield:
        flatfield_directory = dest_directory + "\\flat"
        F = mvf.AverageMosaic(source_directory, extension="bmp")
        #skimage.io.imsave(dest_directory + "/flatfield.bmp", F.astype(np.uint8))
        #F = skimage.io.imread(dest_directory + "/flatfield.bmp")
        mvf.FlatfieldMosaic(source_directory, flatfield_directory, gain = 1, flatfield_image=F, extension="bmp")
        source_directory = flatfield_directory
    
    mvt.TiffConvert(source_directory, dest_directory)
  


print("This is a test")
window = tk.Tk()

btn_source = tk.Button(text="Source...", command=set_source)
btn_source.grid(column=0, row=0)
lbl_source = tk.Label(text=source_directory, fg="red")
lbl_source.grid(column=1, row=0)

btn_dest = tk.Button(text="Destination...", command=set_dest)
btn_dest.grid(column=0, row=1)
lbl_dest = tk.Label(text=dest_directory, fg="red")
lbl_dest.grid(column=1, row=1)

chk_align = tk.Checkbutton(text="Flatfield Correction", command=set_flatfield)
chk_align.grid(column=0, row=2)

#btn_align = tk.Button(text="Alignment...", command=set_align)
#btn_align.grid(column=0, row=2)
#lbl_align = tk.Label(text=align_directory, fg="red")
#lbl_align.grid(column=1, row=2)

btn_begin = tk.Button(text="Begin Processing", command=begin_processing)
btn_begin.grid(column=0, row=3)

#lbl_xoverlap = tk.Label(text="X Overlap", textvariable=x_overlap)
#lbl_xoverlap.grid(column=0, row=3)
#ent_xoverlap = tk.Entry(width=10)
#ent_xoverlap.insert(0, str(x_overlap))
#ent_xoverlap.grid(column=1, row=3)

#lbl_yoverlap = tk.Label(text="Y Overlap", textvariable=y_overlap)
#lbl_yoverlap.grid(column=0, row=4)
#ent_yoverlap = tk.Entry(width=10)
#ent_yoverlap.insert(0, str(y_overlap))
#ent_yoverlap.grid(column=1, row=4)

window.mainloop()