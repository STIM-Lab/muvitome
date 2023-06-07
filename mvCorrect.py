import numpy as np
import os
import skimage
import skimage.io
import skimage.color
import matplotlib.pyplot as plt
from tqdm import tqdm
import glob


if os.name == 'nt':
    import win32api, win32con


def file_is_hidden(p):
    if os.name== 'nt':
        attribute = win32api.GetFileAttributes(p)
        return attribute & (win32con.FILE_ATTRIBUTE_HIDDEN | win32con.FILE_ATTRIBUTE_SYSTEM)
    else:
        return p.startswith('.') #linux-osx

def listdir_nohidden(path):
    FileNames = os.listdir(path)
    result = []
    for f in FileNames:
        if not file_is_hidden(path + "/" + f):
            result.append(f)
    return result

def CalculateAverageImageTiles(path):
    # get the list of folders
    TileFolders = os.listdir(path)

    FileCounter = 0
    for fi in tqdm(range(len(TileFolders))):
        
        CurrentFolder = path + "/" + TileFolders[fi]
        
        # get the names of all images in the stack
        ImageNames = listdir_nohidden(CurrentFolder)
    
        
        for ii in range(len(ImageNames)):
            
            image = skimage.io.imread(CurrentFolder + "/" + ImageNames[ii])
            
            if fi == 0 and ii == 0:
                SumImage = np.zeros(image.shape)
                
            SumImage = SumImage + image
            FileCounter = FileCounter + 1
        
    AverageImage = SumImage / FileCounter
    return AverageImage



def CorrectIlluminationTiles(in_path, out_path):
    print("Calculating average image")
    AverageImage = CalculateAverageImageTiles(in_path)
    
    
    
    IntensityRed = np.median(AverageImage[:, :, 0])
    IntensityGreen = np.median(AverageImage[:, :, 1])
    IntensityBlue = np.median(AverageImage[:, :, 2])
    
    TileFolders = os.listdir(in_path)
    
    # create the "correction" folder if it doesn't already exist
    if not os.path.exists(out_path):
        os.mkdir(out_path)
    
    
    print("Applying illumination correction")
    for fi in tqdm(range(len(TileFolders))):
        
        # get the names of all images in the stack
        ImageNames = listdir_nohidden(in_path + "/" + TileFolders[fi])
    
        InputFolder = in_path + "/" + TileFolders[fi]
        CorrectionFolder = out_path + "/" + TileFolders[fi]
        for ii in range(len(ImageNames)):
            
            image = skimage.io.imread(InputFolder + "/" + ImageNames[ii])
            
            ratio = image.astype(np.float64) / AverageImage
            corrected = ratio
            corrected[:, :, 0] = ratio[:, :, 0] * IntensityRed
            corrected[:, :, 1] = ratio[:, :, 1] * IntensityGreen
            corrected[:, :, 2] = ratio[:, :, 2] * IntensityBlue
            
            corrected = corrected.astype(np.uint8)
            
            
            if not os.path.exists(CorrectionFolder):
                os.mkdir(CorrectionFolder)
            skimage.io.imsave(CorrectionFolder + "/" + ImageNames[ii], corrected)

def CalculateAverageImage(path):
        
    # get the names of all images in the path folder
    ImageNames = listdir_nohidden(path)

    FileCounter = 0
    for ii in range(len(ImageNames)):
        
        image = skimage.io.imread(path + "/" + ImageNames[ii])
        
        if ii == 0:
            SumImage = np.zeros(image.shape)
        else:  
            SumImage = SumImage + image
        FileCounter = FileCounter + 1
        
    AverageImage = SumImage / FileCounter
    return AverageImage

def CorrectIllumination(in_path, out_path):
    print("Calculating average image")
    AverageImage = CalculateAverageImage(in_path)    
    
    
    IntensityRed = np.median(AverageImage[:, :, 0])
    IntensityGreen = np.median(AverageImage[:, :, 1])
    IntensityBlue = np.median(AverageImage[:, :, 2])
       
    # create the "correction" folder if it doesn't already exist
    if not os.path.exists(out_path):
        os.mkdir(out_path)
    
        
    # get the names of all images in the stack
    ImageNames = listdir_nohidden(in_path)

    InputFolder = in_path
    CorrectionFolder = out_path
    for ii in tqdm(range(len(ImageNames))):
        
        image = skimage.io.imread(InputFolder + "/" + ImageNames[ii])
        
        ratio = image.astype(np.float64) / AverageImage
        corrected = ratio
        corrected[:, :, 0] = ratio[:, :, 0] * IntensityRed
        corrected[:, :, 1] = ratio[:, :, 1] * IntensityGreen
        corrected[:, :, 2] = ratio[:, :, 2] * IntensityBlue
        
        corrected = corrected.astype(np.uint8)
        
        
        if not os.path.exists(CorrectionFolder):
            os.mkdir(CorrectionFolder)
        skimage.io.imsave(CorrectionFolder + "/" + ImageNames[ii], corrected)
    return AverageImage