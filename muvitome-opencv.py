# -*- coding: utf-8 -*-
"""
Created on Wed Nov 30 16:39:18 2022

@author: david
"""

import numpy as np
import os
import skimage
import skimage.io
import cv2 as cv
import matplotlib.pyplot as plt

path = "D:/Dropbox/source/david-muvitome/stitching"

folders = os.listdir(path)

index_str = folders[0].split("_")


Columns = []
Rows = []
Images = []

for f in folders:
    indices = [int(s) for s in f.split("_") if s.isdigit()]
    Columns.append(indices[0])
    Rows.append(indices[1])
    
    I = cv.imread(path + "/" + f + "/00000.bmp")
    #Igray = skimage.color.rgb2gray(I).astype(np.float32)
    Images.append(I)
    


start = 1
stop = 4
Images = Images[start:stop+1]
Columns = np.array(Columns[start:stop+1])
Rows = np.array(Rows[start:stop+1])

t = stop - start + 1
for i in range(t):
    
    plt.subplot(t, 1, i + 1)
    plt.imshow(Images[i], vmin=np.min(Images), vmax=np.max(Images))

stitcher = cv.Stitcher.create(cv.Stitcher_SCANS)
status, stitched = stitcher.stitch(Images)


if status == cv.STITCHER_ERR_NEED_MORE_IMGS:
    print("ERROR: Need more images")