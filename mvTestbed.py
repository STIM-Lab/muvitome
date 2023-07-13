import mvAlign as mva
import mvCorrect as mvc
import mvMosaic as mvm
import tkinter as tk
from tkinter import filedialog as fd
import os
import matplotlib.pyplot as plt

source_directory = "C:/Users/david/Dropbox/todo/muvitome_phantoms/liver_1um_15x"
dest_directory = "C:/Users/david/Dropbox/todo/muvitome_phantoms/liver"

#%%

# assemble the mosaic image
#MosaicFolder = dest_directory + "\\mosaic"
#mvm.AssembleMuvitome(source_directory, MosaicFolder, (5, 0))

#AlignedFolder = dest_directory + "\\aligned"

offsets = mva.CalculateImageOffsetListY(source_directory, N=500, sigma=0)
mva.ApplyImageOffsetsY(source_directory, dest_directory, offsets)

plt.plot(offsets)

