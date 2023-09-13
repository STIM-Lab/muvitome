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

# specify the source and destination directory
# (note that the destination directory will by default contain several copies of the files)

source_directory = "D:/2023_08_15_11_21_0_395NeuNA594"
dest_directory = "D:/2023_08_15_11_21_0_395NeuNA594_processed"
if not os.path.exists(dest_directory):
    os.mkdir(dest_directory)

#%%
# FLATFIELD CORRECTION
# This code calculates a flatfield image for correction by averaging all images in the source directory
# The flatfield image is stored in the destination directory as a bitmap

F = mvf.AverageMosaic(source_directory, extension="bmp")
skimage.io.imsave(dest_directory + "/flatfield.bmp", F.astype(np.uint8))


#%%
# This code performs flatfield correction by loading the flatfield image and correcting all images in the source directory
# This creates a separate directory in destination called "flat"
F = skimage.io.imread(dest_directory + "/flatfield.bmp")
mvf.FlatfieldMosaic(source_directory, dest_directory + "/flat", gain = 1, flatfield_image=F, extension="bmp")

#%%
# BUILD MOSAIC
# This code assembles all of the corrected images into a mosaic stored in the "mosaic" directory"
MosaicFolder = dest_directory + "/mosaic"
mvm.AssembleMuvitome(dest_directory + "/flat", MosaicFolder, (10, 0))

#%%
# ALIGN IMAGES
# This code aligns the images and stores the result as a new image stack in "aligned"
offsets = mva.CalculateImageOffsetListY(dest_directory + "/mosaic", N=100, sigma=3)
mva.ApplyImageOffsetsY(dest_directory + "/mosaic", dest_directory + "/aligned", offsets)
