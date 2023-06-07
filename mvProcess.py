import mvAlign as mva
import mvCorrect as mvc
import mvMosaic as mvm
import tkinter as tk
from tkinter import filedialog as fd
import os

source_directory = "[Source Directory Not Selected]"
dest_directory = "[Destination Directory Not Selected]"
align_directory = "[Alignment Stack Not Selected]"
y_overlap = 0
x_overlap = 5

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
    
def set_align():
    global align_directory
    align_directory = fd.askdirectory()
    lbl_align["text"] = align_directory
    lbl_align["fg"] = "black"
    
def begin_processing():
    global source_directory
    global dest_directory
    global x_overlap
    global y_overlap
    
    # assemble the mosaic image
    MosaicFolder = dest_directory + "\\mosaic"
    mvm.AssembleMuvitome(source_directory, MosaicFolder, (float(x_overlap), float(y_overlap)))

    AlignedFolder = dest_directory + "\\aligned"
    offsets = mva.CalculateImageOffsetListY(align_directory, N=100, sigma=0)
    mva.ApplyImageOffsetsY(MosaicFolder, AlignedFolder, offsets)
    
    


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

btn_align = tk.Button(text="Destination...", command=set_align)
btn_align.grid(column=0, row=2)
lbl_align = tk.Label(text=align_directory, fg="red")
lbl_align.grid(column=1, row=2)

btn_begin = tk.Button(text="Begin Processing", command=begin_processing)
btn_begin.grid(column=2, row=2)

lbl_xoverlap = tk.Label(text="X Overlap", textvariable=x_overlap)
lbl_xoverlap.grid(column=0, row=3)
ent_xoverlap = tk.Entry(width=10)
ent_xoverlap.insert(0, str(x_overlap))
ent_xoverlap.grid(column=1, row=3)

lbl_yoverlap = tk.Label(text="Y Overlap", textvariable=y_overlap)
lbl_yoverlap.grid(column=0, row=4)
ent_yoverlap = tk.Entry(width=10)
ent_yoverlap.insert(0, str(y_overlap))
ent_yoverlap.grid(column=1, row=4)

window.mainloop()