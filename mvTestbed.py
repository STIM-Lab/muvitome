import mvAlign as mva
import mvCorrect as mvc
import mvMosaic as mvm
import mvFlatfield as mvf
import tkinter as tk
from tkinter import filedialog as fd
import os
import matplotlib.pyplot as plt
import skimage
import numpy as np

source_directory = "D:/2023_08_24_16_56_35_david_rebooted"
dest_directory = "D:/2023_08_24_16_56_35_david_rebooted_processed"

if not os.path.exists(dest_directory):
    os.mkdir(dest_directory)

#%%
# FLATFIELD CORRECTION

F = mvf.AverageMosaic(source_directory, extension="bmp")
skimage.io.imsave(dest_directory + "/flatfield.bmp", F.astype(np.uint8))


#%%
F = skimage.io.imread(dest_directory + "/flatfield.bmp")

mvf.FlatfieldMosaic(source_directory, dest_directory + "/flat", gain = 1, flatfield_image=F, extension="bmp")

#%%
# assemble the mosaic image
MosaicFolder = dest_directory + "/mosaic"
mvm.AssembleMuvitome(dest_directory + "/flat", MosaicFolder, (10, 0))

#%%
offsets = mva.CalculateImageOffsetListY(dest_directory + "/flat/00002_00004", N=100, sigma=3)
mva.ApplyImageOffsetsY(dest_directory + "/mosaic", dest_directory + "/aligned", offsets)
