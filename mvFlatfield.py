import skimage.io
import numpy as np
import scipy as sp
import matplotlib.pyplot as plt
from os import listdir
from os.path import isfile, join
import os
from tqdm import tqdm
import glob

def AverageStack(image_directory, extension=None):
    
    if extension is None:
        ImageNames = os.listdir(image_directory)
    else:
        ImageNames = []
        for file in os.listdir(image_directory):
            if file.endswith("." + extension):
                ImageNames.append(file)

    
    #ImageNames = os.listdir(image_directory)
    for ii in tqdm(range(len(ImageNames))):
        
        
        imagename = ImageNames[ii]
        image = skimage.io.imread(image_directory + "/" + imagename)
        
        # if this is the first image, load it into the sum image
        if(ii == 0):
            sum_image = image.astype(np.float64)
        else:
            sum_image = sum_image + image.astype(np.float64)
    
    return sum_image / len(ImageNames)
    
def AverageMosaic(mosaic_directory, extension=None):
    
    TileFolders = os.listdir(mosaic_directory)
    
    for ti in (range(len(TileFolders))):
        print(str(ti) + "/" + str(len(TileFolders)) + "----------------------")
        tilefolder = TileFolders[ti]
        avg_tile = AverageStack(mosaic_directory + "/" + tilefolder, extension)
        if(ti == 0):
            sum_tiles = avg_tile
        else:
            sum_tiles = sum_tiles + avg_tile
            
    return sum_tiles / len(TileFolders)

def FlatfieldStack(in_path, out_path, gain = 1.0, flatfield_image = None, extension=None):
    
    # if no flatfield image is provided, calculate one
    if flatfield_image is None:
        F = AverageStack(in_path)
    else:
        F = flatfield_image.astype(np.float64)
    
    # create the output directory if it doesn't exist
    if not os.path.exists(out_path):
        os.mkdir(out_path)
        
    m = np.average(np.average(F, 0), 0)
    G = m / F * gain
    
    # correct each image in the directory
    if extension is None:
        ImageNames = os.listdir(in_path)
    else:
        ImageNames = []
        for file in os.listdir(in_path):
            if file.endswith("." + extension):
                ImageNames.append(file)
    for ii in tqdm(range(len(ImageNames))):        
        
        imagename = ImageNames[ii]
        image = skimage.io.imread(in_path + "/" + imagename)
        
        C = image * G
        
        skimage.io.imsave(out_path + "/" + imagename, C.astype(np.uint8), check_contrast=False)
        
def FlatfieldMosaic(in_path, out_path, gain = 1.0, flatfield_image = None, extension=None):
    
    # create the output directory if it doesn't exist
    if not os.path.exists(out_path):
        os.mkdir(out_path)
        
    if flatfield_image is None:
        F = AverageMosaic(in_path)
    else:
        F = flatfield_image.astype(np.float64)
        
    TileFolders = os.listdir(in_path)
    for ti in (range(len(TileFolders))):
        print(str(ti) + "/" + str(len(TileFolders)) + "----------------------")
        tilefolder = TileFolders[ti]
        
        # create the tile directory if it doesn't exist
        if not os.path.exists(out_path + "/" + tilefolder):
            os.mkdir(out_path + "/" + tilefolder)
            
        FlatfieldStack(in_path + "/" + tilefolder, out_path + "/" + tilefolder, gain, F, extension)
            
