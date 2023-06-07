import mvAlign as mva
import mvCorrect as mvc
import mvMosaic as mvm
import tkinter as tk
from tkinter import filedialog as fd
import os
import matplotlib.pyplot as plt

source_directory = "D:/Dropbox/2023_05_18_10_33_6_ZebrafishGulfWax"
dest_directory = "C:/Users/david/Desktop/processed3"

#%%

# assemble the mosaic image
#MosaicFolder = dest_directory + "\\mosaic"
#mvm.AssembleMuvitome(source_directory, MosaicFolder, (5, 0))

#AlignedFolder = dest_directory + "\\aligned"
offsets0 = mva.CalculateImageOffsetListY(source_directory + "/00001_00002", N=100, sigma=0)
offsets1 = mva.CalculateImageOffsetListY(source_directory + "/00001_00003", N=100, sigma=0)
offsets2 = mva.CalculateImageOffsetListY(source_directory + "/00001_00004", N=100, sigma=0)
offsets3 = mva.CalculateImageOffsetListY(source_directory + "/00002_00002", N=100, sigma=0)
offsets4 = mva.CalculateImageOffsetListY(source_directory + "/00002_00003", N=100, sigma=0)
offsets5 = mva.CalculateImageOffsetListY(source_directory + "/00002_00004", N=100, sigma=0)
#mva.ApplyImageOffsetsY(MosaicFolder, AlignedFolder, offsets)

#%%
plt.plot(offsets0, label="00001_00002")
plt.plot(offsets1, label="00001_00003")
plt.plot(offsets2, label="00001_00004")
plt.plot(offsets3, label="00002_00002")
plt.plot(offsets4, label="00002_00003")
plt.plot(offsets5, label="00002_00004")
plt.legend()

